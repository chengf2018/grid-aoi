local c = require "gridaoi.c"

local aoi = {}

--id:地图id,width:地图宽度,high:地图高度,gridwidth:多少单位一格子
function aoi:new(id, width, high, gridwidth)
	local ctx = setmetatable({}, {
		__index = self,
		__gc = function (ctx)
			assert(ctx.core ~= nil)
			c.aoi_delete(ctx.core)
		end
		}
	)
	ctx.id = id
	ctx.core = c.aoi_new(width, high, gridwidth, 1)
	return ctx
end

--进入aoi
--id:场景中的唯一标示,pos:当前位置,layer:在aoi中的层,viewLayer:能看到的层
function aoi:enter(id, pos, layer, viewLayer)
	return c.aoi_enter(self.core, id, pos.x, pos.z, layer, viewLayer)
end

function aoi:leave(id)
	return c.aoi_leave(self.core, id)
end

function aoi:update(id, pos)
	local enter, leave = c.aoi_update(self.core, id, pos.x, pos.z)
	return enter, enter, leave, leave
end

function aoi:viewlist(id)
	return c.aoi_viewlist(self.core, id)
end

return aoi
