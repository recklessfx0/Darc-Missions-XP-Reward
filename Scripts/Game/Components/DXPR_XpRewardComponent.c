/**
 * DXPR_XpRewardComponent.c
 *
 * Game-mode component that watches DarcMissions' mission frame
 * and rewards nearby players when a mission is won.
 */

class DXPR_XpRewardComponentClass : ScriptComponentClass {}

class DXPR_XpRewardComponent : ScriptComponent
{
    [Attribute("500", UIWidgets.EditBox, "Default XP when no JSON config found")]
    protected int m_iDefaultXpFallback;

    [Attribute("100", UIWidgets.EditBox, "Radius (m) around mission center to find players who receive XP")]
    protected float m_fRewardRadius;

    [Attribute("10000", UIWidgets.EditBox, "How often to check DarcMissions mission state, in milliseconds")]
    protected int m_iPollIntervalMs;

    [Attribute("1000", UIWidgets.EditBox, "Fallback conversion when only vanilla rank component is available")]
    protected int m_iXpPerRank;

    protected ref DXPR_Config m_Config;
    protected ref array<string> m_RewardedMissionIds = {};

    protected const string CONFIG_FILENAME = "dc_xpRewardConfig.json";
    protected const int CONFIG_JSON_VERSION = 1;

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        if (!Replication.IsServer())
            return;

        LoadConfig();
        StartMissionWatcher();
    }

    protected void LoadConfig()
    {
        m_Config = new DXPR_Config();
        m_Config.defaultXp = m_iDefaultXpFallback;

        SDRC_JsonApi2 jsonApi = new SDRC_JsonApi2(CONFIG_FILENAME);
        bool ok = jsonApi.Load(m_Config, SDRC_Config.Cast(m_Config), CONFIG_JSON_VERSION, true, true);

        if (!ok)
            Print("[DXPR] Failed to load " + CONFIG_FILENAME + ". Using in-memory defaults.", LogLevel.WARNING);

        if (!m_Config.missionXpOverrides)
            m_Config.missionXpOverrides = {};
    }

    protected void StartMissionWatcher()
    {
        if (m_iPollIntervalMs <= 0)
            m_iPollIntervalMs = 10000;

        GetGame().GetCallqueue().CallLater(CheckCompletedMissions, m_iPollIntervalMs, true);
        Print("[DXPR] Watching DarcMissions mission frame for completed missions.", LogLevel.NORMAL);
    }

    protected void CheckCompletedMissions()
    {
        SCR_BaseGameMode baseGameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
        if (!baseGameMode || !baseGameMode.missionFrame)
            return;

        foreach (SDRC_Mission mission : baseGameMode.missionFrame.m_MissionList)
        {
            TryRewardMission(mission);
        }
    }

    protected void TryRewardMission(SDRC_Mission mission)
    {
        if (!mission)
            return;

        string missionId = mission.GetId();
        if (m_RewardedMissionIds.Find(missionId) >= 0)
            return;

        if (mission.GetSuccess() != SDRC_EMissionSuccess.WIN)
            return;

        int xpToAward = mission.GetXP();
        if (xpToAward <= 0)
            xpToAward = m_Config.GetXpForMission(mission.GetType());

        SCR_BaseGameMode baseGameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
        if (baseGameMode && baseGameMode.missionFrame)
        {
            int difficulty = mission.GetDifficulty();
            if (difficulty >= 0 && difficulty < baseGameMode.missionFrame.m_Config.missionDifficulty.rewardCoef.Count())
                xpToAward = Math.Round(xpToAward * baseGameMode.missionFrame.m_Config.missionDifficulty.rewardCoef[difficulty]);
        }

        if (xpToAward <= 0)
            return;

        m_RewardedMissionIds.Insert(missionId);
        AwardXpToNearbyPlayers(mission.GetPos(), xpToAward, mission);
    }

    protected void AwardXpToNearbyPlayers(vector pos, int xp, SDRC_Mission mission)
    {
        array<int> playerIds = new array<int>();
        GetGame().GetPlayerManager().GetAllPlayers(playerIds);

        int rewarded = 0;

        foreach (int playerId : playerIds)
        {
            IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
            if (!playerEntity)
                continue;

            if (vector.Distance(playerEntity.GetOrigin(), pos) > m_fRewardRadius)
                continue;

            GiveXpToPlayer(playerId, playerEntity, xp);

            if (m_Config.showXpHint)
                SendXpHint(xp, mission);

            rewarded++;
        }

        Print(string.Format("[DXPR] Awarded %1 XP to %2 player(s) for completing mission %3",
            xp, rewarded, m_Config.GetMissionTypeName(mission.GetType())),
            LogLevel.NORMAL);
    }

    protected void GiveXpToPlayer(int playerId, IEntity player, int xp)
    {
        SCR_XPHandlerComponent xpHandler = SCR_XPHandlerComponent.Cast(
            GetGame().GetGameMode().FindComponent(SCR_XPHandlerComponent));

        if (xpHandler)
        {
            xpHandler.AwardXP(playerId, SCR_EXPRewards.BASE_SEIZED, 1.0, false, xp);
            return;
        }

        SCR_CharacterRankComponent rankComp = SCR_CharacterRankComponent.Cast(
            player.FindComponent(SCR_CharacterRankComponent));

        if (!rankComp)
        {
            Print("[DXPR] SCR_CharacterRankComponent not found on player - no vanilla rank reward applied.", LogLevel.WARNING);
            return;
        }

        if (m_iXpPerRank <= 0)
            m_iXpPerRank = 1000;

        int rankSteps = xp / m_iXpPerRank;
        if (rankSteps < 1)
            rankSteps = 1;

        int currentRank = SCR_CharacterRankComponent.GetCharacterRank(player);
        rankComp.SetCharacterRank(currentRank + rankSteps);
    }

    protected void SendXpHint(int xp, SDRC_Mission mission)
    {
        string missionName = m_Config.GetMissionTypeName(mission.GetType());
        string msg = string.Format("Mission complete! +%1 XP [%2]", xp, missionName);
        SDRC_MissionHintHelper.Show("XP Reward", msg);
    }
}
