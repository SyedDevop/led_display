# The default could be '/dev/ttyACM0'

dev=$(ls /dev/ttyACM* | head -n 1)
echo "Using $dev"
wezterm start -- sudo minicom -b 115200 -D $dev
