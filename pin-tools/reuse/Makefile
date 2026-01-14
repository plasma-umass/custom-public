# PIN_ROOT should point to the Pin installation directory.
CONFIG_ROOT := $(PIN_ROOT)/source/tools/Config
include $(CONFIG_ROOT)/makefile.config

TOOL_ROOTS := reuse
TOOL_LDFLAGS += -z noexecstack

include $(TOOLS_ROOT)/Config/makefile.default.rules
