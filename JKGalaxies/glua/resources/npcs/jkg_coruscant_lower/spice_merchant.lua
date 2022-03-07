NPC.NPCName = "spice_merchant"

function NPC:OnInit(spawner)
	self.Spawner = spawner
end

function NPC:OnSpawn()
	-- Initial setup
	-- Prevent the AI from messing things up
	self:SetBehaviorState("BS_CINEMATIC") 
	self.GodMode = true
	self.NoKnockback = true
	self.LookForEnemies = false
	self.ChaseEnemies = false
	
	--vendor setup
	self:MakeVendor("spicevendor")
	self:RefreshVendorStock()
	self.UseRange = 150 -- make us easier to use
	
	--local vars
	self.LastUse = 0
end

function NPC:OnUse(other, activator)
	if sys.Time() - self.LastUse < 500 then
		return
	end
	self.LastUse = sys.Time()
	
	if not activator:IsPlayer() then
		return		-- Only talk to players, nothin else
	end
	
	local ply = activator:ToPlayer()
	self:UseVendor(ply)
	
end

function NPC:OnTouch(other)
	self:SetAnimBoth("BOTH_STAND10TOSTAND1") --if we get bumped into, react
end

function NPC:OnRemove()
	self.Spawner:Use(self, self)
end
