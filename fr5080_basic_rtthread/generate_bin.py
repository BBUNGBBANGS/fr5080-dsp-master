import os, struct, re

PROJECT_NAME = "fr5080_basic_rtthread_xip"

FIND_TEXT_START = 1
FIND_TEXT_END = 2
FIND_RO_START = 3
FIND_DATA_END = 4
FIND_END = 5

sections = [
	[0, 0, "text.data"],
	[0, 0, "ro_rw.data"],
]

text_start_pattern = re.compile(r"\s+(0x[0-9 a-f]{8})\s+_ResetVector_text_start = ABSOLUTE (.).*")
text_end_pattern = re.compile(r"\s+(0x[0-9 a-f]{8})\s+_text_end = ABSOLUTE (.).*")
ro_start_pattern = re.compile(r"\s+(0x[0-9 a-f]{8})\s+_rodata_start = ABSOLUTE (.).*")
data_end_pattern = re.compile(r"\s+(0x[0-9 a-f]{8})\s+_data_end = ABSOLUTE (.).*")

find_state = FIND_TEXT_START
f_map = open("bin/frbt_hifi3/Release/%s.map" % PROJECT_NAME)
for line in f_map.readlines():
    if find_state == FIND_TEXT_START:
        match_obj = text_start_pattern.match(line)
        if match_obj != None:
            find_state = FIND_TEXT_END
            text_start = int(match_obj.group(1), 16)
            text_start = text_start + 3
            text_start = text_start & 0xffffffffc
            sections[0][0] = text_start
    elif find_state == FIND_TEXT_END:
        match_obj = text_end_pattern.match(line)
        if match_obj != None:
            find_state = FIND_RO_START
            data_length = int(match_obj.group(1), 16)
            data_length = data_length + 3
            data_length = data_length & 0xffffffffc
            sections[0][1] = data_length - sections[0][0]
    elif find_state == FIND_RO_START:
        match_obj = ro_start_pattern.match(line)
        if match_obj != None:
            find_state = FIND_DATA_END
            ro_start = int(match_obj.group(1), 16)
            ro_start = ro_start + 3
            ro_start = ro_start & 0xffffffffc
            sections[1][0] = ro_start
    elif find_state == FIND_DATA_END:
        match_obj = data_end_pattern.match(line)
        if match_obj != None:
            find_state = FIND_END
            data_length = int(match_obj.group(1), 16)
            data_length = data_length + 3
            data_length = data_length & 0xffffffffc
            sections[1][1] = data_length - sections[1][0]
            break
f_map.close()

print sections

apply_env_cmd = r"C:\usr\xtensa\XtDevTools\install\tools\RG-2017.8-win32\XtensaTools\Tools\misc\xtensaenv.bat C:\usr\xtensa\XtDevTools\install\builds\RG-2017.8-win32\hifi3_bd5 C:\usr\xtensa\XtDevTools\install\tools\RG-2017.8-win32\XtensaTools C:\usr\xtensa\XtDevTools\install\builds\RG-2017.8-win32\hifi3_bd5\config hifi3_bd5"

for start, length, name in sections:
    apply_env_cmd = apply_env_cmd + r" & xt-dumpelf --base=0x%08x --width=8 --little-endian --full bin/frbt_hifi3/Release/%s --size=0x%08x > %s" % (start, PROJECT_NAME, length, name)
    
print apply_env_cmd
os.system(apply_env_cmd)

f_out = open("%s.bin" % PROJECT_NAME, 'wb')
for start, length, name in sections:
    f_out.write(struct.pack("I", start))
    f_out.write(struct.pack("I", length))
    f_in = open(name, 'r')
    count = 0
    for line in f_in.readlines():
        count = count + 1
        length = len(line)
        if length < 2:
            continue
        f_out.write(struct.pack("B", int(line[0:2], 16)))
    f_in.close()
f_out.close()

