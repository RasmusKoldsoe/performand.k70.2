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
	{ 0xd7cf1d7e, "i2c_master_send" },
	{ 0x256b9790, "cdev_del" },
	{ 0xdd6b1cc1, "i2c_del_driver" },
	{ 0x1a9df6cc, "malloc_sizes" },
	{ 0xb0bb9c02, "down_interruptible" },
	{ 0x77d921ed, "device_destroy" },
	{ 0x7e0404f0, "__register_chrdev" },
	{ 0x4100e1eb, "i2c_put_adapter" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x3c2c5af5, "sprintf" },
	{ 0x7d11c268, "jiffies" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xfa2a45e, "__memzero" },
	{ 0x5f754e5a, "memset" },
	{ 0xea147363, "printk" },
	{ 0x71c90087, "memcmp" },
	{ 0x6b37f373, "i2c_unregister_device" },
	{ 0xa145aa70, "i2c_register_driver" },
	{ 0xbb4f6c53, "kmem_cache_alloc" },
	{ 0x108e8985, "param_get_uint" },
	{ 0xf96c3baf, "i2c_master_recv" },
	{ 0x37a0cba, "kfree" },
	{ 0x9d669763, "memcpy" },
	{ 0x3285cc48, "param_set_uint" },
	{ 0x8cf51d15, "up" },
	{ 0x5d714057, "class_destroy" },
	{ 0xc585d298, "i2c_get_adapter" },
	{ 0x1b972840, "i2c_new_device" },
	{ 0xe914e41e, "strcpy" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "888CB870AF48B5917EE5F5C");
