import subprocess as sp
from datetime import datetime as dt
import os
import sys

def upload_cycle(num=1):
    if not os.path.exists("finalvids"):
        os.makedirs("finalvids")
    for n in range(num):
        work_name = dt.strftime(dt.now(), 'DDAB%Y-%m-%d-%H-%M-%S-%f')
        out_path = os.path.join("finalvids", work_name+".mp4")
        sp.call(["audio/mus.exe", "audio.wav"])
        sp.call(["video/render.exe", "-f", str(30*20)])
        sp.call(["ffmpeg", "-y", "-r", "30", "-i", "%04d.ppm", "-pix_fmt", "yuv420p", "-c:v", "libx264", "video.mp4"])
        sp.call(["ffmpeg", "-y", "-i", "audio.wav", "-stream_loop", "-1", "-i", "video.mp4",\
            "-c:v", "copy", "-c:a", "aac", "-strict", "experimental", "-pix_fmt", "yuv420p", "-shortest", out_path])
        #sp.call(["python", "upload.py", '--file='+out_path, '--description=Have a good day.', '--title='+work_name, \
        #        '--keywords=gaming,meme,science,programming,humanity,humanism,insect,animals', '--category=29', '--privacyStatus=public'])
        os.system("del *.ppm")
    
if __name__=="__main__":
    runtimes = 1
    if len(sys.argv)>1:
        runtimes = int(sys.argv[1])
    upload_cycle(num=runtimes)