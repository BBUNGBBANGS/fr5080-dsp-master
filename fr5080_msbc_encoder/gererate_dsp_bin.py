PROJECT_NAME = "fr5080_dsp_total"

#source_table = ("fr5080_basic.bin", "fr5080_call.bin", "fr5080_aac.bin", "fr5080_mp3_2_sbc.bin")
source_table = ("fr5080_basic.bin",  "fr5080_call.bin", "", "","fr5080_native_playback.bin","fr5080_msbc_decoder.bin","fr5080_msbc_encoder.bin","")

f_out = open("%s.bin" % PROJECT_NAME, "wb")

for file in source_table:
        if file == "":
            f_out.write("freqchip".encode('ascii'))
        else:        
            f_in = open(file, "rb")
            while True:
                c = f_in.read(1)
                if c:
                    f_out.write(c)
                else:
                    break
            f_in.close()

f_out.write("FREQCHIP".encode('ascii'))	
f_out.close()
