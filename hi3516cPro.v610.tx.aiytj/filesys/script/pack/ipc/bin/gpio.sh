#! /bin/sh

fn_gpio_cat()
{
    local PIN=${1:-82}
    [ ! -e "/sys/class/gpio/gpio$PIN" ] && echo $PIN > /sys/class/gpio/export
    echo in > /sys/class/gpio/gpio$PIN/direction
    cat /sys/class/gpio/gpio$PIN/value
}

fn_gpio_put()
{
    local PIN=${1:-82}
    [ ! -e "/sys/class/gpio/gpio$PIN" ] && echo $PIN > /sys/class/gpio/export
    echo out > /sys/class/gpio/gpio$PIN/direction
    echo ${2:-1} > /sys/class/gpio/gpio$PIN/value               # high active                                                                                                                
}

fn_main()
{
    case $1 in
    cat)
        fn_gpio_cat $2
        ;;
    put)
        fn_gpio_put $2 $3
        ;;
    *)
        echo "
        gpio.sh cat NUM             # read  PIN NUM
        gpio.sh put NUM {0|1}       # write PIN NUM
        "
        ;;
    esac
}

fn_main $@

