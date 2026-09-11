/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"

#include "../eventdriver.h"

/* platform */
#include "../../detail/portable.h"

#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <linux/input.h>

#define __cdecl

#define DEVICE_NONE 0

#define INPUT_BUTTON_FIRST 1
#define INPUT_BUTTON_LAST 127
#define INPUT_MOVE_UP 128
#define INPUT_MOVE_DOWN 129
#define INPUT_MOVE_LEFT 130
#define INPUT_MOVE_RIGHT 131

const char* buttonNames[32] = {
	"TRIGGER",
	"THUMB",
	"THUMB2",
	"TOP",
	"TOP2",
	"PINKIE",
	"BASE",
	"BASE2",
	"BASE3",
	"BASE4",
	"BASE5",
	"BASE6",
	"","","",
	"DEAD",

	"SOUTH",
	"EAST",
	"C",
	"NORTH",
	"WEST",
	"Z",
	"TL",
	"TR",
	"TL2",
	"TR2",
	"SELECT",
	"START",
	"MODE",
	"THUMBL",
	"THUMBR"
};

const char* axisNames[32] = {
	"X",
	"Y",
	"Z",
	"RX",
	"RY",
	"RZ",
	"THROTTLE",
	"RUDDER",
	"WHEEL",
	"GAS",
	"BRAKE",
	"","","","","",
	"HAT0X",
	"HAT0Y",
	"HAT1X",
	"HAT1Y",
	"HAT2X",
	"HAT2Y",
	"HAT3X",
	"HAT3Y",
	"PRESSURE",
	"DISTANCE",
	"TILT_X",
	"TILT_Y",
	"TOOL_WIDTH"
};

typedef struct Axis
{
	int min;
	int max;
	float value;
} Axis;

#define maxButtons 32
#define maxAxes 32

typedef struct Joystick
{	

	bool connected;
	bool buttons[maxButtons];
	Axis axes[maxAxes];
	char name[128];
	int file;
	bool hasRumble;
	short rumbleEffectID;
} Joystick;

void openJoysticks(Joystick* out_joysticks, unsigned int maxJoysticks)
{
	char fileName[32];
	for (int i=0; i<32; ++i) {
		sprintf(fileName, "/dev/input/event%d", i);
		int file = open(fileName, O_RDWR | O_NONBLOCK);
		if (file != -1)
		{
			Joystick j = {0};
			j.connected = true;
			j.file = file;

			// Get name
			ioctl(file, EVIOCGNAME(sizeof(j.name)), j.name);

			// Setup axes
			for (unsigned int i=0; i< maxAxes; ++i)
			{
				struct input_absinfo axisInfo;
				if (ioctl(file, EVIOCGABS(i), &axisInfo) != -1)
				{
					j.axes[i].min = axisInfo.minimum;
					j.axes[i].max = axisInfo.maximum;
				}
			}

			// Setup rumble
			struct ff_effect effect = {0};
			effect.type = FF_RUMBLE;
			effect.id = -1;
			if (ioctl(file, EVIOCSFF, &effect) != -1) {
				j.rumbleEffectID = effect.id;
				j.hasRumble = true;
			}

			out_joysticks[i] = j;
		}
	}
}

void closeJoysticks(Joystick* joysticks, unsigned int maxJoysticks)
{
	for (int i=0; i<32; ++i) {
		if (joysticks[i].connected) {
			close(joysticks[i].file);
			joysticks[i].connected = false;
		}
	}
}

void readJoystickInput(Joystick* joystick)
{
	struct input_event event;
	
	while (read(joystick->file, &event, sizeof(event)) > 0)
	{
		if (event.type == EV_KEY && event.code >= BTN_JOYSTICK && event.code <= BTN_THUMBR) {
			joystick->buttons[event.code-0x120] = event.value;
		}
		if (event.type == EV_ABS && event.code < ABS_TOOL_WIDTH) {
			Axis* axis = &joystick->axes[event.code];
			float normalized = (event.value - axis->min) / (float)(axis->max - axis->min) * 2 - 1;
			joystick->axes[event.code].value = normalized;
		}
	}
}


Joystick joysticks[32] = {0};
int deviceChangeNotify;

typedef struct psy_EVJoystickDriver {
	psy_EventDriver driver;		
	int (*error)(int, const char*);
	psy_EventDriverInput lastinput;	
	bool running_;	
	bool stop_requested_;	
	psy_Property* devices;
	psy_Property* configuration;
} psy_EVJoystickDriver;

static void driver_free(psy_EventDriver*);
static int driver_init(psy_EventDriver*);
static int driver_open(psy_EventDriver*);
static int driver_close(psy_EventDriver*);
static int driver_dispose(psy_EventDriver*);
static const psy_EventDriverInfo* driver_info(psy_EventDriver*);
static void driver_configure(psy_EventDriver*, const psy_Property*);
static const psy_Property* driver_configuration(const psy_EventDriver*);
static void driver_cmd(psy_EventDriver*, const char* section,
	psy_EventDriverInput, psy_EventDriverCmd*);
static psy_EventDriverCmd driver_getcmd(psy_EventDriver*, const char* section);
static void driver_setcmddefaults(psy_EVJoystickDriver*, psy_Property* cmddef);
static void setcmddef(psy_EventDriver* driver, const psy_Property* cmddef)
{
	psy_EVJoystickDriver* self = (psy_EVJoystickDriver*)driver;

	/*if (cmddef && cmddef->children) {
		if (self->cmddef) {
			psy_property_remove(self->configuration, self->cmddef);
		}
		self->cmddef = psy_property_clone(cmddef);
		psy_property_append_property(self->configuration,
			self->cmddef);
		psy_property_set_text(self->cmddef, "cmds.keymap");
		driver_setcmddefaults(self, self->cmddef);
	}*/		
	if (cmddef && cmddef->children) {
		psy_EVJoystickDriver* self = (psy_EVJoystickDriver*)driver;
		psy_Property* cmds;


		cmds = psy_property_at(self->configuration, "cmds", PSY_PROPERTY_TYPE_NONE);
		if (cmds) {
			psy_property_remove(self->configuration, cmds);
		}
		cmds = psy_property_append_property(self->configuration,
			psy_property_clone(cmddef));
		psy_property_set_text(cmds, "cmds.keymap");
		driver_setcmddefaults(self, cmds);
	}		
}


void driver_setcmddefaults(psy_EVJoystickDriver* self, psy_Property* cmddef)
{
	psy_Property* section;

	section = psy_property_find(cmddef, "general", PSY_PROPERTY_TYPE_SECTION);
	if (section) {
		psy_property_set_int(section, "cmd_editmachine", INPUT_BUTTON_FIRST + 1);
		psy_property_set_int(section, "cmd_editpattern", INPUT_BUTTON_FIRST + 2);
		psy_property_set_int(section, "cmd_help", INPUT_BUTTON_FIRST + 3);
		psy_property_set_int(section, "cmd_loadsong", INPUT_BUTTON_FIRST + 4);
	}
	section = psy_property_find(cmddef, "tracker", PSY_PROPERTY_TYPE_SECTION);
	if (section) {
		psy_property_set_int(section, "navup", INPUT_MOVE_UP);
		psy_property_set_int(section, "navdown", INPUT_MOVE_DOWN);
		psy_property_set_int(section, "navleft", INPUT_MOVE_LEFT);
		psy_property_set_int(section, "navright", INPUT_MOVE_RIGHT);
	}
	section = psy_property_find(cmddef, "edit", PSY_PROPERTY_TYPE_SECTION);
	if (section) {
		psy_property_set_int(section, "selectmachine", INPUT_BUTTON_FIRST + 4);
	}	
	section = psy_property_find(cmddef, "notes", PSY_PROPERTY_TYPE_SECTION);
	if (section) {
		psy_property_set_int(section, "cmd_note_default", INPUT_BUTTON_FIRST + 0);
	}	
}

static void driver_idle(psy_EventDriver* driver)
{	
	psy_EVJoystickDriver* self;

	assert(driver);
	
	self = (psy_EVJoystickDriver*)driver;
	if (self->running_)
	{
		// Update which joysticks are connected
		struct inotify_event event;
		if (read(deviceChangeNotify, &event, sizeof(event)+16) != -1)
		{
			closeJoysticks(joysticks, 32);
			openJoysticks(joysticks, 32);
		}

		// Update and print inputs for each connected joystick
		for (unsigned int i=0; i<32; ++i)
		{
			if (joysticks[i].connected)
			{
				readJoystickInput(&joysticks[i]);

				printf("%s - Axes: ", joysticks[i].name);
				for (char axisIndex=0; axisIndex< maxAxes; ++axisIndex) {
					if (joysticks[i].axes[axisIndex].max-joysticks[i].axes[axisIndex].min) printf("%s:% f ", axisNames[axisIndex], joysticks[i].axes[axisIndex].value);
				}
				if (joysticks[i].axes[0].value > 0.1) {
							self->lastinput.message = TRUE;
					self->lastinput.param1 = INPUT_MOVE_RIGHT;
					self->lastinput.param2 = 1;
					if (driver->callback) {
						driver->callback(driver->callbackcontext, driver);
					}
				} else if (joysticks[i].axes[0].value < -0.1) {
					self->lastinput.message = TRUE;
					self->lastinput.param1 = INPUT_MOVE_LEFT;
					self->lastinput.param2 = -1;
					if (driver->callback) {
						driver->callback(driver->callbackcontext, driver);
					}
				}
				if (joysticks[i].axes[1].value > 0.1) {
							self->lastinput.message = TRUE;
					self->lastinput.param1 = INPUT_MOVE_DOWN;
					self->lastinput.param2 = 1;
					if (driver->callback) {
						driver->callback(driver->callbackcontext, driver);
					}
				} else if (joysticks[i].axes[1].value < -0.1) {
					self->lastinput.message = TRUE;
					self->lastinput.param1 = INPUT_MOVE_UP;
					self->lastinput.param2 = -1;
					if (driver->callback) {
						driver->callback(driver->callbackcontext, driver);
					}
				}
				printf("Buttons: ");
				for (char buttonIndex=0; buttonIndex< maxButtons; ++buttonIndex) {
					if (joysticks[i].buttons[buttonIndex]) printf("%s ", buttonNames[buttonIndex]);
				}
				printf("\n");
				if (joysticks[i].buttons[0]) {
					self->lastinput.message = TRUE;
					self->lastinput.param1 = INPUT_BUTTON_FIRST;
					self->lastinput.param2 = 0;
					if (driver->callback) {
						driver->callback(driver->callbackcontext, driver);
					}
				}
				short weakRumble   = fabsf(joysticks[i].axes[ABS_X].value) * 0xFFFF;
				short strongRumble = fabsf(joysticks[i].axes[ABS_Y].value) * 0xFFFF;
				// setJoystickRumble(joysticks[i], weakRumble, strongRumble);
			}
		}
		fflush(stdout);
		usleep(16000);
	}	
	
	
}
static void process_event(psy_EVJoystickDriver*);
static void print_udev_info(void);

static psy_EventDriverInput driver_input(psy_EventDriver* context)
{
	psy_EVJoystickDriver* self = (psy_EVJoystickDriver*)context;
	return self->lastinput;
}

static void driver_makeconfig(psy_EventDriver*);
static int onerror(int err, const char* msg);

static void MidiCallback(void* driver);

static psy_EventDriverVTable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(void)
{
	if (!vtable_initialized) {
		vtable.open = driver_open;
		vtable.deallocate = driver_free;
		vtable.open = driver_open;
		vtable.close = driver_close;
		vtable.dispose = driver_dispose;
		vtable.info = driver_info;
		vtable.configure = driver_configure;
		vtable.configuration = driver_configuration;
		vtable.error = onerror;
		vtable.cmd = driver_cmd;
		vtable.getcmd = driver_getcmd;		
		vtable.setcmddef = setcmddef;
		vtable.idle = driver_idle;
		vtable.input = driver_input;
		vtable_initialized = TRUE;
	}
}

int onerror(int err, const char* msg)
{
	fprintf(stderr, "Udev joystick driver %s", msg);
	return 0;	
}

EXPORT psy_EventDriverInfo const *  psy_eventdriver_moduleinfo(void)
{
	static psy_EventDriverInfo info;

	info.guid = PSY_EVENTDRIVER_EVJOYSTICK_GUID;
	info.Flags = 0;
	info.Name = "Evdev Joystick Driver";
	info.ShortName = "evdevjoystick";
	info.Version = 0;
	return &info;
}

EXPORT psy_EventDriver* __cdecl psy_eventdriver_create(void)
{
	psy_EVJoystickDriver* mme;
	
	mme = (psy_EVJoystickDriver*)malloc(sizeof(psy_EVJoystickDriver));
	if (mme) {			
		driver_init(&mme->driver);
		return &mme->driver;
	}
	return NULL;
}

void driver_free(psy_EventDriver* driver)
{
	free(driver);
}

int driver_init(psy_EventDriver* driver)
{
	psy_EVJoystickDriver* self;
	
	assert(driver);

	self = (psy_EVJoystickDriver*)driver;
	memset(self, 0, sizeof(psy_EVJoystickDriver));
	vtable_init();
	self->driver.vtable = &vtable;
	// self->deviceid = DEVICE_NONE;
	// self->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	driver_makeconfig(&self->driver);
	self->driver.callback = NULL;
	self->driver.callbackcontext = NULL;
	self->lastinput.message = -1;
	self->running_ = FALSE;
	self->stop_requested_ = FALSE;	
	return 0;
}

int driver_dispose(psy_EventDriver* driver)
{
	psy_EVJoystickDriver* self;

	assert(driver);
	
	self = (psy_EVJoystickDriver*)driver;
	// if (self->seq_handle) {
		driver_close(driver);
	// }
	psy_property_deallocate(self->configuration);
	self->configuration = NULL;	
	// CloseHandle(self->hEvent);	
	return 0;
}

const psy_EventDriverInfo* driver_info(psy_EventDriver* self)
{
	return psy_eventdriver_moduleinfo();
}

void driver_makeconfig(psy_EventDriver* context)
{		
	psy_EVJoystickDriver* self;
	psy_Property* devices;
	char key[256];
	uint32_t i;
	uint32_t n;

	self = (psy_EVJoystickDriver*)context;		
	
	psy_snprintf(key, 256, "edjoystick-guid-%d", PSY_EVENTDRIVER_EVJOYSTICK_GUID);
	self->configuration = psy_property_preventtranslate(
		psy_property_allocinit_key(key));
	psy_property_hide(psy_property_append_int(self->configuration,
		"guid", PSY_EVENTDRIVER_EVJOYSTICK_GUID, 0, 0));	
	psy_property_set_text(
		psy_property_append_str(self->configuration, "name", "evdev joystick"),
		"settings.name");	
	psy_property_set_text(
		psy_property_append_str(self->configuration, "version", "1.0"),
		"settings.version");
	self->devices = psy_property_append_choice(self->configuration, "device", 0);
	psy_property_append_int(self->devices, "0:None", 0, 0, 0);
	
	/*if (FAILED(hr = self->di->lpVtbl->EnumDevices(self->di,
			DI8DEVCLASS_GAMECTRL, enumCallback,
			self, DIEDFL_ATTACHEDONLY))) {
	}*/
	psy_property_set_text(
		psy_property_append_section(self->configuration, "cmds"),
		"cmds.keymap");	
}

void driver_configure(psy_EventDriver* driver, const psy_Property* configuration)
{	
	psy_EVJoystickDriver* self;
	psy_Property* p;
	
	self = (psy_EVJoystickDriver*)driver;
	if (configuration && self->configuration) {
		psy_property_sync(self->configuration, configuration);		
	}
	if (self->configuration) {
		psy_Property* p;

		p = psy_property_at(self->configuration, "device",
			PSY_PROPERTY_TYPE_NONE);
		if (p) {
			// self->deviceid = (uint32_t)psy_property_item_int(p);
		}
	}
}

int driver_open(psy_EventDriver* driver)
{
	psy_EVJoystickDriver* self = (psy_EVJoystickDriver*)driver;	
	int success = 1;	
		
	if (self->running_) return 0;

	self->lastinput.message = -1;
	
	openJoysticks(joysticks, 32);
	deviceChangeNotify = inotify_init1(IN_NONBLOCK);
	inotify_add_watch(deviceChangeNotify, "/dev/input", IN_ATTRIB);
	
	self->running_ = TRUE;
	return success;	
}

int driver_close(psy_EventDriver* driver)
{
	psy_EVJoystickDriver* self = (psy_EVJoystickDriver*)driver;
	int success = 1;

	self->stop_requested_ = TRUE;
	closeJoysticks(joysticks, 32);
	self->running_ = FALSE;
	return success;
}

void driver_cmd(psy_EventDriver* driver, const char* sectionname,
	psy_EventDriverInput input, psy_EventDriverCmd* cmd)
{	
	psy_EVJoystickDriver* self = (psy_EVJoystickDriver*)driver;	
	psy_Property* section;

	if (!sectionname) {
		return;
	}
	cmd->type = psy_EVENTDRIVER_CMD;
	cmd->id = -1;	
	section = psy_property_findsection(self->configuration, sectionname);
	if (!section) {
		return;
	}
	if (input.message != FALSE) {
		psy_Property* property = NULL;
		psy_List* p;

		for (p = psy_property_begin(section); p != NULL;
				psy_list_next(&p)) {			
			property = (psy_Property*)psy_list_entry(p);			
			if (psy_property_item_int(property) == input.param1) {
				break;
			}
			property = NULL;
		}
		if (property) {
			cmd->id = property->item.id;			
		}
	}
}

psy_EventDriverCmd driver_getcmd(psy_EventDriver* driver, const char* section)
{		
	psy_EventDriverCmd cmd;	
	psy_EVJoystickDriver* self = (psy_EVJoystickDriver*)driver;	
			
	driver_cmd(driver, section, self->lastinput, &cmd);		
	return cmd;
}

const psy_Property* driver_configuration(const psy_EventDriver* driver)
{
	psy_EVJoystickDriver* self;

	self = (psy_EVJoystickDriver*)driver;
	return self->configuration;
}

