ifdef CONFIG_DEBUG_UART
obj-y += debug_uart.o
endif

ifdef CONFIG_SPL_BUILD
obj-y += nuc980_spl.o
ifdef CONFIG_SYS_USE_SPIFLASH
obj-y += wb_spinand.o
else
# TODO: Rewrite
obj-y	+= spl_nand_boot.o
obj-y	+= my_nand_base.o
obj-y	+= my_nuc980_nand.o
obj-y	+= my_nand.o
obj-y	+= my_nand_ecc.o
obj-y	+= my_nand_ids.o
endif
# TODO: Add common files between SPL and full boot
else
obj-y += nuc980.o
endif