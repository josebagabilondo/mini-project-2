APPLICATION = mini-project

BOARD ?= iotlab-m3

RIOTBASE ?= $(CURDIR)/../RIOT

USEMODULE += lps331ap
USEMODULE += xtimer
USEMODULE += netdev_default
USEMODULE += netutils

LWIP ?= 0

ifeq (0,$(LWIP))
  USEMODULE += auto_init_gnrc_netif
  # Specify the mandatory networking modules
  USEMODULE += gnrc_ipv6_default
  # Additional networking modules that can be dropped if not needed
  USEMODULE += gnrc_icmpv6_echo
else
  USEMODULE += lwip_ipv6
  USEMODULE += lwip_netdev
endif

USEMODULE += gcoap

DEVELHELP ?= 1

include $(RIOTBASE)/Makefile.include
