#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x1640806b, "module_layout" },
	{ 0x108e8985, "param_get_uint" },
	{ 0x3285cc48, "param_set_uint" },
	{ 0x47c2bef6, "spi_add_device" },
	{ 0x73e20c1c, "strlcpy" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x634dbbc8, "bus_find_device_by_name" },
	{ 0x701d0ebd, "snprintf" },
	{ 0xb1f06f38, "put_device" },
	{ 0x9a136dd8, "spi_alloc_device" },
	{ 0x26681ec, "spi_busnum_to_master" },
	{ 0x285187ae, "spi_register_driver" },
	{ 0xc34dfa74, "__register_chrdev" },
	{ 0xea147363, "printk" },
	{ 0x5f754e5a, "memset" },
	{ 0xe9c838d5, "spi_sync" },
	{ 0xfa2a45e, "__memzero" },
	{ 0x9d669763, "memcpy" },
	{ 0x97255bdf, "strlen" },
	{ 0x3c2c5af5, "sprintf" },
	{ 0xbb4f6c53, "kmem_cache_alloc" },
	{ 0x1a9df6cc, "malloc_sizes" },
	{ 0x37a0cba, "kfree" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x888a9429, "cdev_del" },
	{ 0x5d714057, "class_destroy" },
	{ 0x77d921ed, "device_destroy" },
	{ 0x22878e33, "driver_unregister" },
	{ 0x818c431b, "device_unregister" },
	{ 0x8cf51d15, "up" },
	{ 0xb0bb9c02, "down_interruptible" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "F19DFFC7F0B5559D09035C4");
