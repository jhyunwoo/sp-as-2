#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x8788a568, "remove_proc_entry" },
	{ 0xde338d9a, "_raw_spin_lock" },
	{ 0xcb8b6ec6, "kfree" },
	{ 0x2f6b9c7e, "pv_ops" },
	{ 0xd272d446, "BUG_func" },
	{ 0x124ac38b, "seq_printf" },
	{ 0x888b8f57, "strcmp" },
	{ 0x9dd73c64, "const_current_task" },
	{ 0xf96799d5, "__tracepoint_mmap_lock_start_locking" },
	{ 0xa59da3c0, "down_read" },
	{ 0xf96799d5, "__tracepoint_mmap_lock_acquire_returned" },
	{ 0xf296206e, "pgdir_shift" },
	{ 0xb1ad3f2f, "boot_cpu_data" },
	{ 0xf96799d5, "__tracepoint_mmap_lock_released" },
	{ 0xa59da3c0, "up_read" },
	{ 0x2dde6814, "__mmap_lock_do_trace_acquire_returned" },
	{ 0xfb27c109, "__mmap_lock_do_trace_start_locking" },
	{ 0x095159b2, "physical_mask" },
	{ 0x1bdf2bc8, "sme_me_mask" },
	{ 0xf296206e, "ptrs_per_p4d" },
	{ 0xbd03ed67, "page_offset_base" },
	{ 0xfb27c109, "__mmap_lock_do_trace_released" },
	{ 0x82fd7238, "__ubsan_handle_shift_out_of_bounds" },
	{ 0xbd03ed67, "random_kmalloc_seed" },
	{ 0xadb999ff, "kmalloc_caches" },
	{ 0x52e49154, "__kmalloc_cache_noprof" },
	{ 0xbd03ed67, "phys_base" },
	{ 0xe7269688, "seq_read" },
	{ 0xfce1f64c, "seq_lseek" },
	{ 0x6d8bd5c2, "single_release" },
	{ 0xd272d446, "__fentry__" },
	{ 0x73369fdb, "proc_create" },
	{ 0xe8213e80, "_printk" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0xc7eeeeb0, "single_open" },
	{ 0x00bc5fb3, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0x8788a568,
	0xde338d9a,
	0xcb8b6ec6,
	0x2f6b9c7e,
	0xd272d446,
	0x124ac38b,
	0x888b8f57,
	0x9dd73c64,
	0xf96799d5,
	0xa59da3c0,
	0xf96799d5,
	0xf296206e,
	0xb1ad3f2f,
	0xf96799d5,
	0xa59da3c0,
	0x2dde6814,
	0xfb27c109,
	0x095159b2,
	0x1bdf2bc8,
	0xf296206e,
	0xbd03ed67,
	0xfb27c109,
	0x82fd7238,
	0xbd03ed67,
	0xadb999ff,
	0x52e49154,
	0xbd03ed67,
	0xe7269688,
	0xfce1f64c,
	0x6d8bd5c2,
	0xd272d446,
	0x73369fdb,
	0xe8213e80,
	0xd272d446,
	0xc7eeeeb0,
	0x00bc5fb3,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"remove_proc_entry\0"
	"_raw_spin_lock\0"
	"kfree\0"
	"pv_ops\0"
	"BUG_func\0"
	"seq_printf\0"
	"strcmp\0"
	"const_current_task\0"
	"__tracepoint_mmap_lock_start_locking\0"
	"down_read\0"
	"__tracepoint_mmap_lock_acquire_returned\0"
	"pgdir_shift\0"
	"boot_cpu_data\0"
	"__tracepoint_mmap_lock_released\0"
	"up_read\0"
	"__mmap_lock_do_trace_acquire_returned\0"
	"__mmap_lock_do_trace_start_locking\0"
	"physical_mask\0"
	"sme_me_mask\0"
	"ptrs_per_p4d\0"
	"page_offset_base\0"
	"__mmap_lock_do_trace_released\0"
	"__ubsan_handle_shift_out_of_bounds\0"
	"random_kmalloc_seed\0"
	"kmalloc_caches\0"
	"__kmalloc_cache_noprof\0"
	"phys_base\0"
	"seq_read\0"
	"seq_lseek\0"
	"single_release\0"
	"__fentry__\0"
	"proc_create\0"
	"_printk\0"
	"__x86_return_thunk\0"
	"single_open\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "5572440F038A6F70CE76EA2");
