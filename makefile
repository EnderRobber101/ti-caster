# ----------------------------
# Makefile Options
# ----------------------------

NAME = TICaster
ICON = icon.png
DESCRIPTION = "3D graphics for the ti-84 plus ce"
COMPRESSED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
