# Nombre de la aplicación
APPLICATION = mini-project-1

# Plataforma de destino (ajusta según tu hardware)
BOARD ?= iotlab-m3

RIOTBASE ?= $(CURDIR)/../RIOT

# Incluir módulos necesarios (por ejemplo, sensores)
USEMODULE += lps331ap
USEMODULE += xtimer

# Incluir el archivo principal de la aplicación
include $(RIOTBASE)/Makefile.include

