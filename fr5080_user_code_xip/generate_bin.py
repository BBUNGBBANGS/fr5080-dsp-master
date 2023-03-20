import os, struct, re

PROJECT_NAME = "fr5080_user_code_xip"

apply_env_cmd = r"C:\usr\xtensa\XtDevTools\install\tools\RG-2017.8-win32\XtensaTools\Tools\misc\xtensaenv.bat C:\usr\xtensa\XtDevTools\install\builds\RG-2017.8-win32\hifi3_bd5 C:\usr\xtensa\XtDevTools\install\tools\RG-2017.8-win32\XtensaTools C:\usr\xtensa\XtDevTools\install\builds\RG-2017.8-win32\hifi3_bd5\config hifi3_bd5"

apply_env_cmd = apply_env_cmd + r" & xt-objcopy -O binary bin/frbt_hifi3/Release/%s %s.bin" % (PROJECT_NAME, PROJECT_NAME)
    
print apply_env_cmd
os.system(apply_env_cmd)


