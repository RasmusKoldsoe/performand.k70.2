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
	{ 0x6980fe91, "param_get_int" },
	{ 0xff964b25, "param_set_int" },
	{ 0x108e8985, "param_get_uint" },
	{ 0x3285cc48, "param_set_uint" },
	{ 0xc34dfa74, "__register_chrdev" },
	{ 0x4100e1eb, "i2c_put_adapter" },
	{ 0x1b972840, "i2c_new_device" },
	{ 0xc585d298, "i2c_get_adapter" },
	{ 0xa145aa70, "i2c_register_driver" },
	{ 0x328a05f1, "strncpy" },
	{ 0x97255bdf, "strlen" },
	{ 0x3c2c5af5, "sprintf" },
	{ 0xf96c3baf, "i2c_master_recv" },
	{ 0xd7cf1d7e, "i2c_master_send" },
	{ 0xf9a475af, "module_put" },
	{ 0xdd6b1cc1, "i2c_del_driver" },
	{ 0x6b37f373, "i2c_unregister_device" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0xea147363, "printk" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "BA8F9FA479491F4B0C4B4DC");
