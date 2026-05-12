--
-- CCUserDefault
--
save = {}

local u = CCUserDefault:sharedUserDefault()

-- get value methods

--[[--
    Get bool value by key, if the key doesn't exist, a default value will return.
    You can set the default value, or it is false.
]]
function save.getBool(key, default)
    local value = u:getBoolForKey(key)
    if default and type(default) == 'boolean' and not value then
        return default
    else
        return u:getBoolForKey(key)
    end
end

--[[--
    Get integer value by key, if the key doesn't exist, a default value will return.
    You can set the default value, or it is 0.
]]
function save.getInteger(key, default)
    local value = u:getIntegerForKey(key)
    if default and type(default) == 'number' and not value then
        return default
    else
        return u:getIntegerForKey(key)
    end
end

--[[--
    Get float value by key, if the key doesn't exist, a default value will return.
    You can set the default value, or it is 0.0f.
]]
function save.getFloat(key, default)
    local value = u:getFloatForKey(key)
    if default and type(default) == 'number' and not value then
        return default
    else
        return u:getFloatForKey(key)
    end
end

--[[--
    Get double value by key, if the key doesn't exist, a default value will return.
    You can set the default value, or it is 0.0.
]]
function save.getDouble(key, default)
    local value = u:getDoubleForKey(key)
    if default and type(default) == 'number' and not value then
        return default
    else
        return u:getDoubleForKey(key)
    end
end

--[[--
    Get string value by key, if the key doesn't exist, a default value will return.
    You can set the default value, or it is ''.
]]
function save.getString(key, default)
    local value = u:getStringForKey(key)
    if default and type(default) == 'string' and not value then
        return default
    else
        return u:getStringForKey(key)
    end
end

-- set value methods

--[[--
    Set bool value by key.
]]
function save.bool(key, bool) u:setBoolForKey(key, bool) end

--[[--
    Set integer value by key.
]]
function save.integer(key, int) u:setIntegerForKey(key, int) end

--[[--
    Set float value by key.
]]
function save.float(key, float) u:setFloatForKey(key, float) end

--[[--
    Set double value by key.
]]
function save.double(key, double) u:setDoubleForKey(key, double) end

--[[--
    Set string value by key.
]]
function save.string(key, str) u:setStringForKey(key, str) end

--[[--
    Save content to xml file
]]
function save.flush() u:flush() end

function save.purgeSharedUserDefault() u:purgeSharedUserDefault() end

function save.getDataFilePath() return u:getXMLFilePath():c_str() end

function save.isDataFileExist() return u:isXMLFileExist() end

--
-- save extensions for NarutoSenki
--
function save.isBGM() return save.getBool('isBGM') end
function save.isVoice() return save.getBool('isVoice') end
function save.isPreload() return save.getBool('isPreload') end
function save.isHardCore() return save.getBool('isHardCore') end

return save
