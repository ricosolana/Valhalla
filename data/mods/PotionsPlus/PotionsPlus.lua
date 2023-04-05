ConfigSync = require 'config'

local peers = {}

local mod = ModManager:GetMod('PotionsPlus')

local config = ConfigSync.new(mod.name, 'PotionsPlus/com.odinplus.potionsplus.cfg', mod.version, mod.version, false)
