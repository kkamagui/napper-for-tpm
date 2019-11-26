#!/usr/bin/python2.7
#-*- coding: utf-8 -*-
#
#                            Napper 
#                         ------------
#        TPM vulnerability checking tool for CVE-2018-6622 
#
#               Copyright (C) 2019 Seunghun Han
#     at National Security Research Institute of South Korea
#    Project link: https://github.com/kkamagui/napper-for-tpm
#
import subprocess
import os
import sys
from time import sleep

# Color codes
RED = '\033[1;31m'
GREEN = '\033[1;32m'
YELLOW = '\033[1;33m'
BLUE = '\033[1;34m'
MAGENTA = '\033[1;35m'
CYAN = '\033[1;36m'
WHITE = '\033[1;37m'
ENDC = '\033[0m'
BOLD = '\033[1m'
UNDERLINE = '\033[4m'
BLINK = '\033[5m'
SUCCESS = GREEN
FAIL = RED

#
# Show banner.
#
def show_banner():
    banner = """\
                     ,----------------,              ,---------,
                ,-----------------------,          ,"        ,"|
              ," """ + GREEN + 'Napper v 1.3 for TPM' + ENDC + """ ,"|        ,"        ,"  |
             +-----------------------+  |      ,"        ,"    |
             |  .-----------------""" + GREEN + BLINK + 'Z' + ENDC + """  |  |     +---------+      |
             |  |               """ + GREEN + BLINK + 'Z' + ENDC + """ |  |  |     | -==----'|      |
             |  |   ︶     ︶ """ + GREEN + BLINK + 'z' + ENDC + """   |  |  |     |         |      |
             |  |       -         |  |  |/----| ==== oo |      |
             |  |                 |  |  |   ,/| ((((    |    ,"
             |  `-----------------'  |," .;'/ | ((((    |  ,"
             +-----------------------+  ;;  | |         |,"     
                /_)______________(_/  //'   | +---------+
           ___________________________/___  `,
          /  oooooooooooooooo  .o.  oooo /    \,"---------
         / ==ooooooooooooooo==.o.  ooo= /    ,`\--{-D)  ,"
         `-----------------------------'    '----------"

""" + \
    GREEN + 'Napper v1.3 for checking a TPM and Intel PTT vulnerability, CVE-2018-6622 and unknown CVE\n' + ENDC + \
    '             Made by Seunghun Han, https://kkamagui.github.io\n' + \
    '         Project link: https://github.com/kkamagui/napper-for-tpm \n'
    print banner

#
# Print colored message
#
def color_print(message, color):
    print color + message + ENDC
    return

#
# Check TPM version in this system
#
def check_tpm_version():
    tpm_version = ""
    print '    [*] Checking TPM version...',
    try:
        output = subprocess.check_output('journalctl -b | grep "TPM"', shell=True)
    except:
        output = ''

    if '2.0 TPM' in output:
        color_print('TPM v2.0.', SUCCESS)
        tpm_version = '2.0'
    elif '1.2 TPM' in output:
        color_print('TPM v1.2.', SUCCESS)
        tpm_version = '1.2'
    elif 'ACPI: TPM2' in output:
        color_print('Intel PTT.', SUCCESS)
        tpm_version = '2.0'
    else:
        print 'No TPM.'

    return tpm_version

#
# Check if the resource manager is running and run it.
#
def check_and_run_resource_manager():
    print '    [*] Checking the resource manager process...',

    try:
        output = subprocess.check_output('ps -e | grep resourcemgr', shell=True)
    except:
        output = ''

    # Already running.
    if 'resourcemgr' in output:
        color_print('Running.', SUCCESS)
        return 0
    else:
        color_print('Starting.', SUCCESS)

    # Start the resource manager
    pid = os.fork()
    if (pid == 0):
        try:
            subprocess.call('resourcemgr > /dev/null', shell=True)
        except:
            print '    [*] resoucemgr error'
        sys.exit(0)
    else:
        sleep(3)

    return 0

#
# Check if the resource manager is running and run it.
#
def check_and_run_vuln_testing_module():
    print '    [*] Checking the TPM vulnerability testing module...',

    try:
        output = subprocess.check_output('lsmod | grep napper', shell=True)
    except:
        output = ''

    # Already running.
    if 'napper' in output:
        color_print("Running.", SUCCESS)
        return 0

    try:
        subprocess.call('insmod napper-driver/napper.ko', shell=True)
        color_print("Starting.", SUCCESS)
    except:
        color_print("Fail.", FAIL)
        return -1
 
    return 0

#
# Sleep system.
#
def sleep_system():
    print '    [*] Ready to sleep! Please press "Enter" key.'
    raw_input ( '    [*] After sleep, please press "Enter" key again to wake up.')
    os.system('systemctl suspend')
    print '\n    [*] Waking up now. Please wait for a while.',
    
    for i in range(0, 10):
        print '.',
        sleep(1)
        sys.stdout.flush()
    print ''

    return

#
# Check if static PCRs (PCR #0~ PCR #16, PCR #23) are all zeros.
#
def check_pcrs_all_zeros():
    print '\n    [*] Reading PCR values of TPM and checking a vulnerability...',
    try:
        output = subprocess.check_output('tpm2_listpcrs -g 0x04', shell=True)
    except:
        return -1, False

    try:
        output_sha256 = subprocess.check_output('tpm2_listpcrs -g 0x0b', shell=True)
        output += output_sha256
    except:
        output += '\n'

    vulnerable = True
    for line in output.splitlines():
        if "PCR_" in line:
            if not ("00 00 00 00 00 00" in line) and \
               not (("PCR_17" in line) or ("PCR_18" in line) or \
                    ("PCR_19" in line) or ("PCR_20" in line) or \
                    ("PCR_21" in line) or ("PCR_22" in line)):
                vulnerable = False

    if (vulnerable == True):
        color_print('Vulnerable.', FAIL)
    else:
        color_print('Not vulnerable', SUCCESS)

    # Show PCR values
    print "    [*] Show all PCR values:",
    for line in output.splitlines():
        print "       ", line

    return 0, vulnerable

#
# Extend 0xdeadbeef values to all static PCRs.
#
def extend_pcrs():
    print '\n    [*] Extending 0xdeadbeef to all static PCRs.'

    try:
        output = subprocess.check_output('tpm2_extendpcrs -g 0x04', shell=True)
    except:
        return -1

    try:
        output_sha256 = subprocess.check_output('tpm2_extendpcrs -g 0x0b', shell=True)
        output += output_sha256
    except:
        output += '\n'

    # Show PCR values
    print "    [*] Show all PCR values:",
    for line in output.splitlines():
        print "       ", line

    return 0

#
# Show system information.
#
def show_system_info():
    print '    [*] TPM v2.0 information.'
    try:
        output = subprocess.check_output('tpm2_getinfo', shell=True)
    except:
        print "    [*] tpm2_getinfo error"
        return -1

    # Show PCR values
    for line in output.splitlines():
        print "       ", line

    print '\n    [*] System information.'
    try:
	    output = subprocess.check_output('dmidecode -s baseboard-manufacturer', shell=True)
	    print '        Baseboard manufacturer: ' + output,
	    output = subprocess.check_output('dmidecode -s baseboard-product-name', shell=True)
	    print '        Baseboard product name: ' + output,
	    output = subprocess.check_output('dmidecode -s baseboard-version', shell=True)
	    print '        Baseboard version: ' + output,
	    output = subprocess.check_output('dmidecode -s bios-vendor', shell=True)
	    print '        BIOS vendor: ' + output,
	    output = subprocess.check_output('dmidecode -s bios-version', shell=True)
	    print '        BIOS version: ' + output,
	    output = subprocess.check_output('dmidecode -s bios-release-date', shell=True)
	    print '        BIOS release date: ' + output,
	    output = subprocess.check_output('dmidecode -s system-manufacturer', shell=True)
	    print '        System manufacturer: ' + output,
	    output = subprocess.check_output('dmidecode -s system-product-name', shell=True)
	    print '        System product name: ' + output,
    except:
        print '    [*] dmidecode error.'

    return 0

#
# Main function.
#
def main():
    # Show banner.
    show_banner()

    color_print('Checking TPM version for testing.', BOLD)
    tpm_version = check_tpm_version()
    if (tpm_version == "2.0"):
        print "    [*] Your system has TPM v2.0, and vulnerability checking is needed."
    elif (tpm_version == "1.2"):
        print "    [*] Your system has TPM v1.2, and it is not vulnerable."
        return 0
    else:
        print "    [*] Your system has no TPM. Thank you for using this tool!"
        return 0

    color_print('\nPreparing for sleep.', BOLD)
    # Load the TPM vunlerability testing module.
    result = check_and_run_vuln_testing_module()
    if (result != 0):
        return result

    #color_print('\nTesting a TPM vulnerability with sleep.', BOLD)
    sleep_system()

    # Run the resource manager.
    result = check_and_run_resource_manager()
    if (result != 0):
        return result

    # Check if all static PCRs are zeros.
    result, vulnerable = check_pcrs_all_zeros()
    if (result != 0):
        return result

    # Extend 0xdeadbeef value to all static PCRs.
    if (vulnerable == True):
        extend_pcrs()

    # Show summary.
    color_print('\nSummary. Please contribute summary below to the Napper project, https://www.github.com/kkamagui/napper-for-tpm.', BOLD)
    print '    [*] Your TPM version is 2.0, and it is',
    if (vulnerable == True):
        color_print('vulnerable.', FAIL + BLINK)
        color_print('        Please download the latest BIOS firmware from the manufacturer\'s site and update it.\n', FAIL)
    else:
        color_print('safe.\n', SUCCESS + BLINK)

    show_system_info()
    return 0

if __name__ == "__main__":
    main()
    print ""

    if (len(sys.argv) == 2 and sys.argv[1] == 'wait'):
        while True:
            sleep(10)
