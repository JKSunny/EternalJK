-- Exported by JKG Dialogue Creator

DLG.Name = "lost_trooper"
DLG.RootNode = "E1"
DLG.Nodes = {
	E1 = {
		Type = 1,
		SubNode = "T5",
		NextNode = "E2",
		HasCondition = true,
		ConditionFunc = function(owner, ply, data)
			--basic operating procedure for starting a quest
			--Check if quests are not setup, 
			--setup if they aren't, then initialize the quest in either case
			local stage = 0
			local qn = "losttrooper" --shorthand for questname
						
			if not ply.Quests or not ply.Quests[qn] then
				if InitQuest(ply, qn) then
					ply.Quests[qn].title = "The Lost Trooper"
			    end
			end
			
			-- Get the state of the quest (or default to 0)
			if ply.Quests[qn] then
			    stage = ply.Quests[qn].stage
			end
						
			-- Return whether state equals 0, if not returns false
			return stage == 0
		end,
	},
	T5 = {
		Type = 2,
		SubNode = "T6",
		Text = "So...cold...",
		Duration = 1500,
		HasCondition = false,
		HasResolver = false,
	},
	T6 = {
		Type = 2,
		SubNode = "S7",
		Text = "What was that?  Is...is someone there?",
		Duration = 3000,
		HasCondition = false,
		HasResolver = false,
	},
	S7 = {
		Type = 6,
		SubNode = "D8",
		ScriptFunc = function(owner, ply, data)
			--update the quest's stage to 1, and notify player
			ply.Quests["losttrooper"].stage = 1
			ply:SendCenterPrint("Started: " .. ply.Quests["losttrooper"].title)
			ply:SendPrint("Started " .. ply.Quests["losttrooper"].title)
		end,
		HasCondition = false,
	},
	D8 = {
		Type = 5,
	},
	E2 = {
		Type = 1,
		SubNode = "T9",
		NextNode = "E3",
		HasCondition = true,
		ConditionFunc = function(owner, ply, data)
			local stage = 1
						
			if ply.Quests["losttrooper"] then
				stage = ply.Quests["losttrooper"].stage
			end
						
			return stage == 1
		end,
	},
	T9 = {
		Type = 2,
		SubNode = "O10",
		Text = "I must be hallucinating...you can't be real, can you?",
		Duration = 3000,
		HasCondition = false,
		HasResolver = false,
	},
	O10 = {
		Type = 3,
		SubNode = "T11",
		NextNode = "O14",
		Text = "I'm definitely real.",
		HasCondition = false,
		HasResolver = false,
	},
	T11 = {
		Type = 2,
		SubNode = "T12",
		Text = "That's just what a hallucination would say!",
		Duration = 2500,
		HasCondition = false,
		HasResolver = false,
	},
	T12 = {
		Type = 2,
		SubNode = "D13",
		Text = "I've got to find a way off this rock before I completely lose it...\nCome on Gary, keep it together. They're not real.",
		Duration = 5000,
		HasCondition = false,
		HasResolver = false,
	},
	D13 = {
		Type = 5,
	},
	O14 = {
		Type = 3,
		SubNode = "T17",
		NextNode = "O15",
		Text = "No, not at all.",
		HasCondition = false,
		HasResolver = false,
	},
	T17 = {
		Type = 2,
		SubNode = "O18",
		Text = "That's-wait...are you real?",
		Duration = 3000,
		HasCondition = false,
		HasResolver = false,
	},
	O18 = {
		Type = 3,
		SubNode = "T23",
		NextNode = "O19",
		Text = "Yes.  My name is $name$.",
		HasCondition = false,
		HasResolver = true,
		ResolveFunc = function(owner, ply, var, data)
			if var == "name" then
				return sys.StripColorcodes(ply.Name)
			end
			return "<name-error>"
		end,
	},
	T23 = {
		Type = 2,
		SubNode = "T24",
		Text = "Oh that's great...just great $name$...",
		Duration = 3000,
		HasCondition = false,
		HasResolver = true,
		ResolveFunc = function(owner, ply, var, data)
			if var == "name" then
				return sys.StripColorcodes(ply.Name)
			end
			return "<name-error>"
		end,
	},
	T24 = {
		Type = 2,
		SubNode = "S25",
		Text = "...damn hallucinations have names now...",
		Duration = 2000,
		HasCondition = false,
		HasResolver = false,
	},
	S25 = {
		Type = 6,
		SubNode = "D26",
		ScriptFunc = function(owner, ply, data)
			ply.Quests["losttrooper"].stage = 2
		end,
		HasCondition = false,
	},
	D26 = {
		Type = 5,
	},
	O19 = {
		Type = 3,
		SubNode = "T27",
		NextNode = "O20",
		Text = "[Persuade]If I shoot you, will the pain convince you I'm real?",
		HasCondition = false,
		HasResolver = false,
	},
	T27 = {
		Type = 2,
		SubNode = "T28",
		Text = "Why do these damn hallucinations make so much sense...",
		Duration = 3000,
		HasCondition = false,
		HasResolver = false,
	},
	T28 = {
		Type = 2,
		SubNode = "D29",
		Text = "Oh...hahaha very funny brain.  \nYou won't trick me, I can imagine pain well enough.",
		Duration = 3000,
		HasCondition = false,
		HasResolver = false,
	},
	D29 = {
		Type = 5,
	},
	O20 = {
		Type = 3,
		SubNode = "T30",
		NextNode = "O21",
		Text = "[Lie]You know who I am.",
		HasCondition = false,
		HasResolver = false,
	},
	T30 = {
		Type = 2,
		SubNode = "T33",
		NextNode = "T31",
		Text = "...TK-313?   Is that you buddy?",
		Duration = 3000,
		HasCondition = true,
		ConditionFunc = function(owner, ply, data)
			if ply:GetCreditCount() > 500 then
			    return true
			end
			return false
		end,
		HasResolver = false,
	},
	T33 = {
		Type = 2,
		SubNode = "D34",
		Text = "Wow, you sure have changed a lot.  Guess the cold rea-h-hang on!\nBlast!  You're just my imagination aren't you?  Why?\nWhy is this happening...",
		Duration = 6000,
		HasCondition = false,
		HasResolver = false,
	},
	D34 = {
		Type = 5,
	},
	T31 = {
		Type = 2,
		SubNode = "D32",
		Text = "Liar!  You're j-just a figment of my imagination!",
		Duration = 3000,
		HasCondition = true,
		ConditionFunc = function(owner, ply, data)
			if ply:GetCreditCount() <= 500 then
			    return true
			end
			return false
		end,
		HasResolver = false,
	},
	D32 = {
		Type = 5,
	},
	O21 = {
		Type = 3,
		SubNode = "D22",
		Text = "Nevermind...",
		HasCondition = false,
		HasResolver = false,
	},
	D22 = {
		Type = 5,
	},
	O15 = {
		Type = 3,
		SubNode = "T35",
		NextNode = "O16",
		Text = "Just another Jedi mind trick I'm afraid...",
		HasCondition = false,
		HasResolver = false,
	},
	T35 = {
		Type = 2,
		SubNode = "T36",
		Text = "O-of course!  That's why it doesn't make any sense!\nA Jedi must be messing with my mind.",
		Duration = 4000,
		HasCondition = false,
		HasResolver = false,
	},
	T36 = {
		Type = 2,
		SubNode = "D37",
		Text = "Otherwise there's no way I could have been the only one to survive in the squad.\nJedi Mind tricks...must be. I've got to find that Jedi!\n",
		Duration = 5000,
		HasCondition = false,
		HasResolver = false,
	},
	D37 = {
		Type = 5,
	},
	O16 = {
		Type = 3,
		SubNode = "T38",
		Text = "[Lie]Snap out of it soldier and give me a report!",
		HasCondition = false,
		HasResolver = false,
	},
	T38 = {
		Type = 2,
		SubNode = "O39",
		Text = "Sir!  There rebels are hiding here they overran our-wait, who are you!?",
		Duration = 4000,
		HasCondition = false,
		HasResolver = false,
	},
	O39 = {
		Type = 3,
		SubNode = "T46",
		NextNode = "O40",
		Text = "A friend.",
		HasCondition = false,
		HasResolver = false,
	},
	T46 = {
		Type = 2,
		SubNode = "T47",
		Text = "I must be losing it...A friend?  Out here?\nJ-just my imagination.  N-nothing r-real out here.",
		Duration = 5000,
		HasCondition = false,
		HasResolver = false,
	},
	T47 = {
		Type = 2,
		SubNode = "D48",
		Text = "Keep it together Gary!  None of these people are real.\nJ-just you...and the cold.",
		Duration = 4000,
		HasCondition = false,
		HasResolver = false,
	},
	D48 = {
		Type = 5,
	},
	O40 = {
		Type = 3,
		SubNode = "L49",
		NextNode = "O41",
		Text = "I'm $name$.",
		HasCondition = false,
		HasResolver = true,
		ResolveFunc = function(owner, ply, var, data)
			--$name$ is "name" here, we should probably strip color codes out
			if var == "name" then
				return sys.StripColorcodes(ply.Name)
			end
			return "<name-error>"
		end,
	},
	L49 = {
		Type = 4,
		Target = "T23",
	},
	O41 = {
		Type = 3,
		SubNode = "T43",
		NextNode = "O42",
		Text = "The last face you'll ever see.",
		HasCondition = false,
		HasResolver = false,
	},
	T43 = {
		Type = 2,
		SubNode = "T44",
		Text = "No...no you're not real!",
		Duration = 3000,
		HasCondition = false,
		HasResolver = false,
	},
	T44 = {
		Type = 2,
		SubNode = "L58",
		Text = "None of you are real!  IT WON'T WORK!\nIT WON'T WORK, DO YOU HEAR!?",
		Duration = 3000,
		HasCondition = false,
		HasResolver = false,
	},
	L58 = {
		Type = 4,
		Target = "T12",
	},
	O42 = {
		Type = 3,
		SubNode = "T50",
		Text = "[Lie]I'm your senior officer private!  Are you questioning me?",
		HasCondition = false,
		HasResolver = false,
	},
	T50 = {
		Type = 2,
		SubNode = "T51",
		Text = "Sir, I would neve-no!  It's a trick!",
		Duration = 3000,
		HasCondition = false,
		HasResolver = false,
	},
	T51 = {
		Type = 2,
		SubNode = "S52",
		Text = "The enemy must be trying to lower our defenses!\nFOR THE EMPIRE!",
		Duration = 4000,
		HasCondition = false,
		HasResolver = false,
	},
	S52 = {
		Type = 6,
		SubNode = "D57",
		ScriptFunc = function(owner, ply, data)
			--make npc vulnerable
			owner.GodMode = false
			owner.NoKnockback = false
			owner.Walking = false
			owner.Running = true
			owner.LookForEnemies = true
			owner.ChaseEnemies = true
			--fail the quest and notify user they failed
			ply.Quests["losttrooper"].stage = -1
			ply:SendCenterPrint("Failed: " .. ply.Quests["losttrooper"].title)
			ply:SendPrint("Failed " .. ply.Quests["losttrooper"].title)
		end,
		HasCondition = false,
	},
	D57 = {
		Type = 5,
	},
	E3 = {
		Type = 1,
		SubNode = "T54",
		NextNode = "E4",
		HasCondition = true,
		ConditionFunc = function(owner, ply, data)
			--player has given Gary their name
			local stage = 0
						
			if ply.Quests["losttrooper"] then
				stage = ply.Quests["losttrooper"].stage
			end
						
			return stage == 2
		end,
	},
	T54 = {
		Type = 2,
		SubNode = "O55",
		Text = "Oh it's you $name$.  You're just part of my imagination, right?\nWhat do you want now?",
		Duration = 4000,
		HasCondition = false,
		HasResolver = true,
		ResolveFunc = function(owner, ply, var, data)
			if var == "name" then
				return sys.StripColorcodes(ply.Name)
			end
			return "<name-error>"
		end,
	},
	O55 = {
		Type = 3,
		SubNode = "D56",
		Text = "Nevermind.",
		HasCondition = false,
		HasResolver = false,
	},
	D56 = {
		Type = 5,
	},
	E4 = {
		Type = 1,
		HasCondition = true,
		ConditionFunc = function(owner, ply, data)
			--Has done next step
			local stage = 0
			
			if ply.Quests["losttrooper"] then
				stage = ply.Quests["losttrooper"].stage
			end
			
			return stage == 3
		end,
	},
}
