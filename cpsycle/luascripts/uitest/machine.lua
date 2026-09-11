-- psycle plugineditor (c) 2023 by psycledelics
-- File: machine.lua
-- copyright 2023 members of the psycle project http://psycle.sourceforge.net
-- This source is free software ; you can redistribute it and/or modify it under
-- the terms of the GNU General Public License as published by the Free Software
-- Foundation ; either version 2, or (at your option) any later version.  

-- require('mobdebug').start()

machine = require("psycle.machine"):new()
  
local component = require("psycle.ui.window")
local label = require("psycle.ui.text")
local listview = require("psycle.ui.listview")
local edit = require("psycle.ui.edit")
local button = require("psycle.ui.button")
local point = require("psycle.ui.point")
local rectangle = require("psycle.ui.rect")
local dimension = require("psycle.ui.dimension")
  
function machine:info()
  return { 
    vendor  = "psycle",
    name    = "UiTest",
    mode    = machine.HOST,
    version = 0,
    api     = 0
  }
end

-- help text displayed by the host
function machine:help()
  return ""
end

function machine:init(samplerate)
   self.component = component:new()
  -- self.label = label:new(self.component):settext("psycle")
   self.listview = listview:new(self.component)
   self.listview:setalign(2)
   local p = point:new(10, 10)
   local that = self
   function self.component:draw(g)
      g:drawline(point:new(0, 0), point:new(1000, 50))
      g:drawstring("text", point:new(10, 10))
      g:fillrect(rectangle:new(point:new(50, 10), dimension:new(10, 10)))
   end
   --function self.button:onclicked()
   --  that.label:settext("pressed")
   --end
   self:setviewport(self.component)
end

return machine
