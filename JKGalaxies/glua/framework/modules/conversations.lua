--[[ ------------------------------------------------
	Jedi Knight Galaxies Lua Framework
	Conversation system/object
	
	DO NOT MODIFY THIS FILE
	
	Written by BobaFett
--------------------------------------------------]]


local PlayerMeta = findmetatable("Player")
if PlayerMeta then	
	function PlayerMeta:StopConversation() 
		self:SendCommand("cin de")
		self:SendCommand("cin stop")
		self:SendCommand("conv stop")
		self:SetCinematicMode(false)
	end
end

	
local function CheckVector(vec, sError)
	if vec ~= nil then 
		if type(vec) == "userdata" then
			if vec.ObjName == "Vector" then 
				return true
			end
		end
	end
	if sError then
		error(sError)
	else
		return false
	end
end

local function CheckEntity(ent, sError)
	if ent ~= nil then 
		if type(ent) == "userdata" then
			if ent.ObjName == "Entity" then 
				return true
			end
		end
	end
	if sError then
		error(sError)
	else
		return false
	end
end

local function CheckPlayer(ply, sError)
	if ply ~= nil then 
		if type(ply) == "userdata" then
			if ply.ObjName == "Player" then 
				return true
			end
		end
	end
	if sError then
		error(sError)
	else
		return false
	end
end


local Conversation = {}

Conversation.__index = Conversation

function sys.CreateConversation()
	local o = {}
	setmetatable(o, Conversation)
	o.Lines = nil
	o.Choices = nil
	o.ChoiceTags = nil
	o.ChoiceCallback = nil
	o.TextEntryCallback = nil
	o.TextEntryCaption = nil
	o.TextEntryDefVal = nil
	o.TextEntryFlags = nil
	o.FinishCallback = nil
	o.UserData = nil
	o.Player = nil
	return o
end

function Conversation:Reset()
	self.Lines = nil
	self.Choices = nil
	self.ChoiceTags = nil
	self.ID = nil
	self.ChoiceCallback = nil
	self.TextEntryCallback = nil
	self.TextEntryCaption = nil
	self.TextEntryDefVal = nil
	self.TextEntryFlags = nil
	self.FinishCallback = nil
	self.Player = nil
end

function Conversation:AddText(text, duration)
	local tbl  = {}
	if (string.len(text) > 600) then
		error("Error: Conversation:AddText - text exceeds 600 characters!")
		return
	end
	
	if not self.Lines then
		-- Initialize the table
		self.Lines = {}
	end
	tbl.text = text
	tbl.duration = duration
	table.insert(self.Lines, tbl)
end

function Conversation:SetChoiceCallback(callbackfunc)
	self.ChoiceCallback = callbackfunc
end

function Conversation:AddChoice(choice, choicetag)
	if not self.Choices then
		-- Initialize the table
		self.Choices = {}
		self.ChoiceTags = {}
	end
	table.insert(self.Choices, choice)
	table.insert(self.ChoiceTags, choicetag)
end

function Conversation:SetTextEntryCallback(callbackfunc)
	self.TextEntryCallback = callbackfunc
end

function Conversation:AddTextEntry(caption, defval, flags)
	self.TextEntryCaption = caption or ""
	self.TextEntryDefVal = defval or ""
	self.TextEntryFlags = flags or 0
end

function Conversation:SetUserData(userdata)
	self.UserData = userdata
end

function Conversation:SetConvoID(ID)
	self.ID = ID
end

function Conversation:GetConvoID()
	return self.ID
end

function Conversation:GetUserData()
	return self.UserData
end

function Conversation:SetFinishCallback(callbackfunc)
	self.FinishCallback = callbackfunc
end

function Conversation:Halt(showMouse)
	-- Halt the convo system, so cinematics can be played
	-- This will put the convo system itself on standby, removing all text and the cursor from screen, until  you continue
	if not self.Player then
		error("Tried to use Continue on a non-activated convo object")
	end
		
	self.Player:SendCommand("cin de")
	if showMouse then
		self.Player:SendCommand("conv haltm")
	else
		self.Player:SendCommand("conv halt")
	end
	self.Halted = true
end

function Conversation:Continue()
	-- Continue an already ongoing conversation
	-- This way we dont have to re-enable the convo system every time we continue
	if not self.Player then
		error("Tried to use Continue on a non-activated convo object")
	end
	if (self.Lines == nil and self.Choices == nil and self.TextEntryCaption == nil) then
		-- No lines, no options and no text entry , abort the convo right away
		if self.FinishCallback then
			if (self.FinishCallback(self, self.Player)) == true then
				return
			end
		end
		self.Player.__sys.CurrConv = nil
		self.Player:StopConversation()
		self.Player.Busy = false;
		return
	end
	self.Player.Busy = true;
	if self.Halted then
		-- Re-enable the convo system if it was halted
		self.Player:SendCommand("cin ae")
		self.Player:SendCommand("conv start")
	end
	
	self.Player:SetEscapeFunc(function(ply)
			timer.Remove(self.TimerName)
			self:ProcessConversation()
		end)
	self:ProcessConversation()
end

function Conversation:Clear()
	-- Clears the convo without totally resetting it
	self.Lines = nil
	self.Choices = nil
	self.ChoiceTags = nil
end


function Conversation:RunConvo(ply)
	if self.Player then
		error("Tried to use RunConvo on an already active convo object")
	end
	CheckPlayer(ply, "Conversation:RunConvo: Invalid player specified")
	self.Player = ply
	if (self.Lines == nil and self.Choices == nil and self.TextEntryCaption == nil) then
		-- No lines, no options and no text entry , abort the convo right away
		if self.FinishCallback then
			if (self.FinishCallback(self, ply)) == true then
				return
			end
		end
		ply.__sys.CurrConv = nil
		ply:StopConversation()
		ply.Busy = false;
		return
	end
	ply.Busy = true;
	ply.__sys.CurrConv = self
	-- Engage cinematic AND conversation mode
	ply:SendCommand("cin start")
	ply:SetCinematicMode(true)
	ply:SendCommand("conv start")
	ply:SendCommand("cin ae")
	self.TimerName = "Con" .. self.Player:GetID()
	self.Halted = false
	ply:SetEscapeFunc(function(ply)
			timer.Remove(self.TimerName)
			self:ProcessConversation()
		end)
	self:ProcessConversation()
end

function Conversation:ProcessConversation()
	-- Main processing function here
	-- First, check if we have pending lines to display, if so, send em now
	local ply = self.Player
	ply.Busy = true;
	local k,v
	if self.Lines then
		for k,v in pairs(self.Lines) do
			-- We got somethin here, display it
			ply:SendCommand(string.format("conv line \"%s\"", v.text))
			if self.TimerName == nil then
				self.TimerName = "Con" .. self.Player:GetID()
				print("Conversation for " .. tostring(self.Player) .. " had no name, assigning " .. self.TimerName .. " to it")
			end
			timer.Create(self.TimerName, v.duration, 1, self.ProcessConversation, self)
			self.Lines[k] = nil
			return
		end
	end
	-- If we get here, we processed all our lines, check if we have choices
	if self.Choices then
		-- Yes we do :P
		-- Because this can contain a lot of text, we'll just place our items in here 'n see how far we can get with it
		local SB = sys.CreateStringBuilder()
		SB:Append(string.format("conv choices %i ", table.getn(self.Choices)))
		for k,v in pairs(self.Choices) do
			if string.len(v) > 200 then
				error("Error: Conversation:ProcessConversation - Oversize conversation choice (> 200 chars)")
				return
			end
			if SB:Length() + string.len(v) > 990 then
				ply:SendCommand(SB:ToString())
				SB:Clear()
				SB:Append("conv cont ")
			end
			SB:Append(string.format("\"%s\" ", v))
		end
		ply:SetEscapeFunc(nil) -- Ignore the escape key
		ply:SendCommand(SB:ToString())
		return
	elseif self.TextEntryCaption then
		-- We got text entry pending
		ply:SetEscapeFunc(nil)
		ply:SendCommand(string.format("conv te \"%s\" \"%s\" %i", self.TextEntryCaption, self.TextEntryDefVal, self.TextEntryFlags))
		return
	end
	-- If we get here, there's no lines and no options left
	ply:SetEscapeFunc(nil)
	if self.FinishCallback then
		self.Lines = nil    -- Reset the convo, so we can put in new info
		self.Choices = nil
		self.ChoiceTags = nil
		if (self.FinishCallback(self, ply)) == true then
			return
		end
	end
	-- If FinishCallback is set and it returns true, keep the convo going, otherwise dont
	ply.__sys.CurrConv = nil
	ply:StopConversation()
	ply.Busy = false;
end

local function ProcessConvoResponse(ply, argc, argv)
	-- We got a responce from the client, call the callback
	local conv = ply.__sys.CurrConv
	local resp = tonumber(argv[1])
	if not conv then
		return
	end
	local resptag = conv.ChoiceTags[resp]
	conv.Lines = nil    -- Reset the convo, so we can put in new info
	conv.Choices = nil
	conv.ChoiceTags = nil
	conv.TextEntryCaption = nil
	if conv.ChoiceCallback then
		conv.ChoiceCallback(conv, ply, conv.ID, resp, resptag)
		return
	end
	-- This shouldn't really happen, but just in case, if no function is set, this'll abort the convo
	ply.__sys.CurrConv = nil
	ply:StopConversation()
	ply.Busy = false;
end

cmds.Add("~convresp", ProcessConvoResponse)

local function ProcessConvoTextEntryResponse(ply, argc, argv)
	-- We got a responce from the client, call the callback
	local conv = ply.__sys.CurrConv
	local resp = tonumber(argv[1])
	local entry = argv[2]
	if not conv then
		return
	end
	conv.Lines = nil    -- Reset the convo, so we can put in new info
	conv.Choices = nil
	conv.ChoiceTags = nil
	conv.TextEntryCaption = nil
	if conv.TextEntryCallback then
		conv.TextEntryCallback(conv, ply, conv.ID, resp != 1, entry)
		return
	end
	-- This shouldn't really happen, but just in case, if no function is set, this'll abort the convo
	ply.__sys.CurrConv = nil
	ply:StopConversation()
	ply.Busy = false;
end

cmds.Add("~convteresp", ProcessConvoTextEntryResponse)