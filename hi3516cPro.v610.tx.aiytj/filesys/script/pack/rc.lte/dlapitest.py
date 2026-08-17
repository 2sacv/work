import os
from ctypes import *

print(os.getcwd())
# load Dll
if os.path.exists('DownloadApi.dll'):
    print('exist')
hDll = CDLL(r"E:\Code\newDownload\demo\DownloadApi.dll")

fun_download = hDll.download_list_api
fun_download.argtypes = [c_char_p,c_char_p,c_char,c_char,c_char,c_char,c_char,c_char]
fun_download.restype = c_int32

# demo
def dl_demo():    
    
    port = "COM11".encode('utf-8')
    cfgPath = "config/cfg_ec618_usb.ini".encode('utf-8')
    ret = c_int32(-1)
    
    try:
        ret = fun_download(port,cfgPath,0,1,1,1,1,0)
    except Exception as e:
        print("Exception: %s" % e)
    print(ret)
    if(ret == 0): #success
        print('download success')
    else:
        print('download fail')

if __name__ == "__main__":
    dl_demo()
