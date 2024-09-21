--[[ ------------------------------------------------
	Jedi Knight Galaxies Lua Framework
	Quest Manager
	
	DO NOT MODIFY THIS FILE
	
	Written by Futuza
    Contains functions useful for managing/testing quests.
--------------------------------------------------]]


--standardized quest constructor--
--to use: InitQuest(ply, "shortquestname"), returns true if successful
function InitQuest(ply, name)
    if name == nil then
        print("Invalid quest name!")
		return false
	end

    --if individual ply.Quest not setup
    if not ply.Quests then
        print("Initializing quest system for " .. ply.Name .. "^7...")
        ply.Quests = {}
        ply.QstCount = 0
    end

    --initialize data
    local data = {}
    local log = {}          --table of objectives/questlog
    data.stage = 0          --what stage we're on
                            --0==not started, 1==started, 2==1st part done, etc.  Can only use numbers.
    data.name = name        --short reference name
    data.title = ""         --display name of the quest
    data.objective = ""     --the current objective of the quest
    data.hidden = false     --if it can be viewed in the datapad/questlog
    data.complete = false   --false = quest is in progress, true = quest is completed
    data.active = true      --true = set as an active quest / false = not on active quest list (can still be completed, but doesn't show on map/markers etc)
    data.questlog = log     --contains a list of all the completed quest objectives

    --assign to player
    ply.Quests[name] = data
    ply.QstCount = ply.QstCount + 1
    print(name .. " quest initialized.")
    return true
end

--lists all the quests in ply.Quests[]
--usage: /listquests
local function ListQuests(ply)
    if ply.Quests == nil then
        ply:SendPrint("Quest system not initialized for this player.  This player has not started any quests.")
        return
    end
    
    print(ply.Name .. "^7 has started " .. ply.QstCount .. " quests:")
    print("|    Quest Title    |   (name)   | stage |")
    print("------------------------------------------")
    if type(ply.Quests) == 'table' then
        for k,v in pairs(ply.Quests) do
            if type(k) ~= 'number' then
                if ply.Quests[k].title ~= nil then 
                    print(ply.Quests[k].title .. " | (" .. ply.Quests[k].name .. ") | " .. ply.Quests[k].stage .. " |")
                else
                    print("<Untitled Quest> | (" .. ply.Quests[k].name ") | " .. ply.Quests[k].stage .. " |")
                end
            end
        end
    end
end

--displays the current objective of a quest
--usage: /quest.getobjective <questname>
local function GetQuestObjective(ply, argc, argv)
    if (argc < 2 or argc > 2) then
		ply:SendPrint("/quest.getstage <questname>")
		return
	end

    if ply.Quests == nil then
        ply:SendPrint("Quest system not initialized for this player.  This player has not started any quests.")
        return
    end

	local questname = argv[1];
	if questname ~= nil then
        if ply.Quests then
            if ply.Quests[questname] then
                ply:SendPrint(ply.Quests[questname].title .. " objective: ")
                ply:SendPrint(ply.Quests[questname].objective)
                return
            end
        else
            ply:SendPrint("^1No active quests.")
            return
        end
	end

    ply:SendPrint("^1Invalid questname specified.")
    return
end


--checks the stage of the specified quest
--usage: /quest.getstage <questshortname>
local function GetQuestStage(ply, argc, argv)
    if (argc < 2 or argc > 2) then
		ply:SendPrint("/quest.getstage <questname>")
		return
	end

    if ply.Quests == nil then
        ply:SendPrint("Quest system not initialized for this player.  This player has not started any quests.")
        return
    end

	local questname = argv[1];
	if questname ~= nil then
        if ply.Quests then
            if ply.Quests[questname] then
                ply:SendPrint(ply.Quests[questname].title .. " is at stage " .. ply.Quests[questname].stage)
                return
            end
        else
            ply:SendPrint("^1No active quests.")
            return
        end
	end

    ply:SendPrint("^1Invalid questname specified.")
    return
end

--Broken function, will change quest stage, but still breaks npcs - needs more work --futuza
local function SetQuestStage(ply, argc, argv)
    if (argc < 3 or argc > 3) then
		ply:SendPrint("/quest.setstage <questname> <stage>")
		return
	end

    if ply.Quests == nil then
        ply:SendPrint("Quest system not initialized for this player.  This player has not started any quests.")
        return
    end

	local questname = argv[1];
	if questname == nil then
		ply:SendPrint("^1Invalid questname specified.")
		return
	end

    local stageval = argv[2];
    if(tonumber(stageval)) then
        if ply.Quests then
            if ply.Quests[questname] then
                ply.Quests[questname].stage = stageval
                ply:SendPrint(ply.Quests[questname].title .. " set to stage " .. ply.Quests[questname].stage)
                return
            else
                ply:SendPrint(questname .. " is not a valid questname or " .. ply.Name .. "^7 has not yet started this quest.")
                return
            end
        end
    end
    ply:SendPrint("^1Invalid stage specified, must be a number.")
    return
end


cmds.Add("quest.getstage", GetQuestStage)
cmds.Add("quest.setstage", SetQuestStage)
cmds.Add("quest.getobjective", GetQuestObjective)
cmds.Add("quest.list", ListQuests)
