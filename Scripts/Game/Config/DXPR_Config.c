/**
 * DXPR_Config.c
 *
 * Configuration class for DarcMissionsXPReward.
 * Loaded by DarcCore's SDRC_JsonApi2 as dc_xpRewardConfig.json.
 */

class DXPR_MissionXpOverride : Managed
{
    string missionType;
    int xp;
}

class DXPR_Config : SDRC_Config
{
    int defaultXp = 500;
    ref array<ref DXPR_MissionXpOverride> missionXpOverrides;
    bool showXpHint = true;
    int hintDuration = 5;

    override void SetDefaults()
    {
        defaultXp = 500;
        showXpHint = true;
        hintDuration = 5;

        missionXpOverrides = {};
        AddMissionXpOverride("HUNTER", 300);
        AddMissionXpOverride("OCCUPATION", 600);
        AddMissionXpOverride("CONVOY", 800);
        AddMissionXpOverride("CRASHSITE", 500);
        AddMissionXpOverride("PATROL", 250);
        AddMissionXpOverride("SQUATTERS", 400);
        AddMissionXpOverride("ROADBLOCK", 350);
        AddMissionXpOverride("HVTVIP", 1500);
        AddMissionXpOverride("HVTITEM", 1200);
        AddMissionXpOverride("STASH", 700);
        AddMissionXpOverride("CHOPPER", 900);
    }

    override bool DoSave(ContainerSerializationSaveContext saveContext, Class T)
    {
        return saveContext.WriteValue("", T);
    }

    int GetXpForMission(SDRC_EMissionType missionType)
    {
        string typeName = GetMissionTypeName(missionType);

        if (missionXpOverrides)
        {
            foreach (DXPR_MissionXpOverride xpOverride : missionXpOverrides)
            {
                if (xpOverride.missionType == typeName)
                    return xpOverride.xp;
            }
        }

        return defaultXp;
    }

    string GetMissionTypeName(SDRC_EMissionType missionType)
    {
        return SCR_Enum.GetEnumName(SDRC_EMissionType, missionType);
    }

    protected void AddMissionXpOverride(string missionType, int xp)
    {
        DXPR_MissionXpOverride xpOverride = new DXPR_MissionXpOverride();
        xpOverride.missionType = missionType;
        xpOverride.xp = xp;
        missionXpOverrides.Insert(xpOverride);
    }
}
