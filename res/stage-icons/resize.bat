mkdir resized
for %%a in ("*.png") do ffmpeg -i "%%a" -vf scale=512:512 -sws_flags neighbor -qp 0 "resized\%%~na.png"