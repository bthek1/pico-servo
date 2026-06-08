default: deploy

pull:
    ssh pi "cd ~/Documents/pico/pico-servo && git pull"

compile:
    ssh pi "cd ~/Documents/pico/pico-servo && ./compile.sh"

compile-clean:
    ssh pi "cd ~/Documents/pico/pico-servo && ./compile.sh --clean"

flash:
    ssh pi "cd ~/Documents/pico/pico-servo && ./flash.sh"

deploy: pull compile flash

monitor:
    ssh pi "picocom -b 115200 /dev/ttyACM0"
