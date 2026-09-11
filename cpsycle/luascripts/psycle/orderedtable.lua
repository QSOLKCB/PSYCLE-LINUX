-- PSYCLE-LINUX provenance-safe ordered-table implementation.
--
-- The r12005 path contained code attributed to a lua-users wiki page
-- without an author/copyright/license grant. This implementation is
-- independently written for PSYCLE-LINUX and preserves the API Psycle
-- uses: new(), hidden(), ipairs(), pairs(), opairs(), and del().
--
-- This source is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2, or (at your option)
-- any later version.

local m = {}

function m.new(t)
   local mt = {}
   local order = {}

   mt.__index = {
      _korder = order,
      hidden = function()
         return pairs(mt.__index)
      end,
      ipairs = function(self)
         return ipairs(self._korder)
      end,
      pairs = function(self)
         return pairs(self)
      end,
      opairs = function(self)
         local i = 0
         local function iter(tbl)
            i = i + 1
            local key = order[i]
            if key ~= nil then
               return key, tbl[key]
            end
         end
         return iter, self
      end,
      del = function(self, key)
         if self[key] then
            rawset(self, key, nil)
            for i, existing in ipairs(order) do
               if existing == key then
                  table.remove(order, i)
                  return
               end
            end
         end
      end,
   }

   mt.__newindex = function(self, key, value)
      if key ~= "del" and value then
         rawset(self, key, value)
         table.insert(order, key)
      end
   end

   return setmetatable(t or {}, mt)
end

return m
