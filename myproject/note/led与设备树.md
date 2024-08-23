
## 设备树文件
### 原始配置dts

gpio: gpio@7e200000 {
			compatible = "brcm,bcm2835-gpio";
			reg = <0x7e200000 0xb4>;
			/*
			 * The GPIO IP block is designed for 3 banks of GPIOs.
			 * Each bank has a GPIO interrupt for itself.
			 * There is an overall "any bank" interrupt.
			 * In order, these are GIC interrupts 17, 18, 19, 20.
			 * Since the BCM2835 only has 2 banks, the 2nd bank
			 * interrupt output appears to be mirrored onto the
			 * 3rd bank's interrupt signal.
			 * So, a bank0 interrupt shows up on 17, 20, and
			 * a bank1 interrupt shows up on 18, 19, 20!
			 */
			interrupts = <2 17>, <2 18>, <2 19>, <2 20>;

			gpio-controller;
			#gpio-cells = <2>;

			interrupt-controller;
			#interrupt-cells = <2>;

			gpio-ranges = <&gpio 0 0 54>;

			/* Defines common pin muxing groups
			 *
			 * While each pin can have its mux selected
			 * for various functions individually, some
			 * groups only make sense to switch to a
			 * particular function together.
			 */
			dpi_gpio0: dpi-gpio0 {
				brcm,pins = <0 1 2 3 4 5 6 7 8 9 10 11
					     12 13 14 15 16 17 18 19
					     20 21 22 23 24 25 26 27>;
				brcm,function = <BCM2835_FSEL_ALT2>;
			};

}
leds: leds {
    compatible = "gpio-leds";

    led_act: led-act {
        label = "ACT";
        default-state = "keep";
        linux,default-trigger = "heartbeat";
    };
};

&led_act {
	gpios = <&gpio 21 GPIO_ACTIVE_HIGH>;
};

&led_act {
	default-state = "on";
	linux,default-trigger = "mmc0";
};

### 编译后反编译dtb得到的dts
alias {

    gpio = "/soc/gpio@7e200000";
}
gpio@7e200000 {
    compatible = "brcm,bcm2711-gpio";
    reg = <0x7e200000 0xb4>;
    interrupts = <0x00 0x71 0x04 0x00 0x72 0x04>;
    gpio-controller;
    #gpio-cells = <0x02>;
    interrupt-controller;
    #interrupt-cells = <0x02>;
    gpio-ranges = <0x07 0x00 0x00 0x3a>;
    gpio-line-names = "ID_SDA\0ID_SCL\0GPIO2\0GPIO3\0GPIO4\0GPIO5\0GPIO6\0GPIO7\0GPIO8\0GPIO9\0GPIO10\0GPIO11\0GPIO12\0GPIO13\0GPIO14\0GPIO15\0GPIO16\0GPIO17\0GPIO18\0GPIO19\0GPIO20\0GPIO21\0GPIO22\0GPIO23\0GPIO24\0GPIO25\0GPIO26\0GPIO27\0RGMII_MDIO\0RGMIO_MDC\0CTS0\0RTS0\0TXD0\0RXD0\0SD1_CLK\0SD1_CMD\0SD1_DATA0\0SD1_DATA1\0SD1_DATA2\0SD1_DATA3\0PWM0_MISO\0PWM1_MOSI\0STATUS_LED_G_CLK\0SPIFLASH_CE_N\0SDA0\0SCL0\0RGMII_RXCLK\0RGMII_RXCTL\0RGMII_RXD0\0RGMII_RXD1\0RGMII_RXD2\0RGMII_RXD3\0RGMII_TXCLK\0RGMII_TXCTL\0RGMII_TXD0\0RGMII_TXD1\0RGMII_TXD2\0RGMII_TXD3";
    phandle = <0x07>;

    dpi-gpio0 {
        brcm,pins = <0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0a 0x0b 0x0c 0x0d 0x0e 0x0f 0x10 0x11 0x12 0x13 0x14 0x15 0x16 0x17 0x18 0x19 0x1a 0x1b>;
        brcm,function = <0x06>;
        phandle = <0x64>;
    };

}

	leds {
		compatible = "gpio-leds";
		phandle = <0xee>;

		led-act {
			label = "ACT";
			default-state = "on";
			linux,default-trigger = "mmc0";
			gpios = <0x07 0x15 0x00>;
			phandle = <0x58>;
		};

		led-pwr {
			label = "PWR";
			gpios = <0x0b 0x02 0x01>;
			default-state = "on";
			linux,default-trigger = "default-on";
			phandle = <0x59>;
		};
	};

每个node都有一个phandle，即node的id，可以通过phandle来引用node。
一个device node在内核中有一个of_node结构体,它有成员phandle,所以可以通过phandle来遍历找到对应的device_node。
```c
struct device_node {
	const char *name;
	phandle phandle;
	const char *full_name;
	struct fwnode_handle fwnode;

	struct	property *properties;
	struct	property *deadprops;	/* removed properties */
	struct	device_node *parent;
	struct	device_node *child;
	struct	device_node *sibling;
#if defined(CONFIG_OF_KOBJ)
	struct	kobject kobj;
#endif
	unsigned long _flags;
	void	*data;
#if defined(CONFIG_SPARC)
	unsigned int unique_id;
	struct of_irq_controller *irq_trans;
#endif
};

```
## 解析gpio属性
led-act的属性gpios = <0x07 0x15 0x00>，即它关联一个phandle 是0x07的device node，参数15是gpio引脚，0x00说明高电平亮灯。
可以通过函数```of_get_named_gpio(node,"gpios",0)```直接得到gpio_desc的索引。
gpio_desc索引对应它所在的gpio_device的descs数组中的一个gpio_desc.即一个引脚
```c
struct gpio_device {
	struct device		dev;
	struct cdev		chrdev;
	int			id;
	struct device		*mockdev;
	struct module		*owner;
	struct gpio_chip	*chip;
	struct gpio_desc	*descs;
	int			base;
	u16			ngpio;
	const char		*label;
	void			*data;
	struct list_head        list;
	struct blocking_notifier_head line_state_notifier;
	struct blocking_notifier_head device_notifier;
	struct rw_semaphore	sem;

#ifdef CONFIG_PINCTRL
	/*
	 * If CONFIG_PINCTRL is enabled, then gpio controllers can optionally
	 * describe the actual pin range which they serve in an SoC. This
	 * information would be used by pinctrl subsystem to configure
	 * corresponding pins for gpio usage.
	 */
	struct list_head pin_ranges;
#endif
};
```

### of_get_named_gpio过程
1，解析led-act的属性gpio,得到gpio属性引用的device_node.
```c
struct of_phandle_args {
	struct device_node *np;
	int args_count;
	uint32_t args[MAX_PHANDLE_ARGS];
};
参数np是led-act的节点。list_name 是属性名字gpio.stem_name
int of_parse_phandle_with_args_map(const struct device_node *np,
				   const char *list_name,
				   const char *stem_name,
				   int index, struct of_phandle_args *out_args)
{

}

```

gpio_device也关联此device_node.

遍历gpiodevices链表，找到对应的gpio_device。
```c
struct gpio_device *gpio_device_find(void *data,
				     int (*match)(struct gpio_chip *gc,
						  void *data))
{
	struct gpio_device *gdev;

	/*
	 * Not yet but in the future the spinlock below will become a mutex.
	 * Annotate this function before anyone tries to use it in interrupt
	 * context like it happened with gpiochip_find().
	 */
	might_sleep();

	guard(spinlock_irqsave)(&gpio_lock);

	list_for_each_entry(gdev, &gpio_devices, list) {
		if (gdev->chip && match(gdev->chip, data))
			return gpio_device_get(gdev);
	}

	return NULL;
}
```

match函数
```c
static int of_gpiochip_match_node_and_xlate(struct gpio_chip *chip, void *data)
{
	struct of_phandle_args *gpiospec = data;

	return device_match_of_node(&chip->gpiodev->dev, gpiospec->np) &&
				chip->of_xlate &&
				chip->of_xlate(chip, gpiospec, NULL) >= 0;
}

```

gpio_chip 和gpio_device 互相引用。gpio_device包含此bank gpio的所有引脚。gpio_chip包含一些操作。

```c
struct gpio_chip {
	const char		*label;
	struct gpio_device	*gpiodev;
	struct device		*parent;
	struct fwnode_handle	*fwnode;
	struct module		*owner;

	int			(*request)(struct gpio_chip *gc,
						unsigned int offset);
	void			(*free)(struct gpio_chip *gc,
						unsigned int offset);
	int			(*get_direction)(struct gpio_chip *gc,
						unsigned int offset);
	int			(*direction_input)(struct gpio_chip *gc,
						unsigned int offset);
	int			(*direction_output)(struct gpio_chip *gc,
						unsigned int offset, int value);
	int			(*get)(struct gpio_chip *gc,
						unsigned int offset);
	int			(*get_multiple)(struct gpio_chip *gc,
						unsigned long *mask,
						unsigned long *bits);
	void			(*set)(struct gpio_chip *gc,
						unsigned int offset, int value);
	void			(*set_multiple)(struct gpio_chip *gc,
						unsigned long *mask,
						unsigned long *bits);
	int			(*set_config)(struct gpio_chip *gc,
					      unsigned int offset,
					      unsigned long config);
	int			(*to_irq)(struct gpio_chip *gc,
						unsigned int offset);

	void			(*dbg_show)(struct seq_file *s,
						struct gpio_chip *gc);

	int			(*init_valid_mask)(struct gpio_chip *gc,
						   unsigned long *valid_mask,
						   unsigned int ngpios);

	int			(*add_pin_ranges)(struct gpio_chip *gc);

	int			(*en_hw_timestamp)(struct gpio_chip *gc,
						   u32 offset,
						   unsigned long flags);
	int			(*dis_hw_timestamp)(struct gpio_chip *gc,
						    u32 offset,
						    unsigned long flags);
	int			base;
	u16			ngpio;
	u16			offset;
	const char		*const *names;
	bool			can_sleep;

#if IS_ENABLED(CONFIG_GPIO_GENERIC)
	unsigned long (*read_reg)(void __iomem *reg);
	void (*write_reg)(void __iomem *reg, unsigned long data);
	bool be_bits;
	void __iomem *reg_dat;
	void __iomem *reg_set;
	void __iomem *reg_clr;
	void __iomem *reg_dir_out;
	void __iomem *reg_dir_in;
	bool bgpio_dir_unreadable;
	int bgpio_bits;
	raw_spinlock_t bgpio_lock;
	unsigned long bgpio_data;
	unsigned long bgpio_dir;
#endif /* CONFIG_GPIO_GENERIC */

#ifdef CONFIG_GPIOLIB_IRQCHIP
	/*
	 * With CONFIG_GPIOLIB_IRQCHIP we get an irqchip inside the gpiolib
	 * to handle IRQs for most practical cases.
	 */

	/**
	 * @irq:
	 *
	 * Integrates interrupt chip functionality with the GPIO chip. Can be
	 * used to handle IRQs for most practical cases.
	 */
	struct gpio_irq_chip irq;
#endif /* CONFIG_GPIOLIB_IRQCHIP */

	/**
	 * @valid_mask:
	 *
	 * If not %NULL, holds bitmask of GPIOs which are valid to be used
	 * from the chip.
	 */
	unsigned long *valid_mask;

#if defined(CONFIG_OF_GPIO)
	/*
	 * If CONFIG_OF_GPIO is enabled, then all GPIO controllers described in
	 * the device tree automatically may have an OF translation
	 */

	/**
	 * @of_gpio_n_cells:
	 *
	 * Number of cells used to form the GPIO specifier.
	 */
	unsigned int of_gpio_n_cells;

	/**
	 * @of_xlate:
	 *
	 * Callback to translate a device tree GPIO specifier into a chip-
	 * relative GPIO number and flags.
	 */
	int (*of_xlate)(struct gpio_chip *gc,
			const struct of_phandle_args *gpiospec, u32 *flags);
#endif /* CONFIG_OF_GPIO */
};
```

## 设置io引脚状态

```c
gpio_direction_output(gpio_num, 1);
gpio_set_value(gpio_num, 1);
```

gpio_to_desc映射索引到gpio_desc结构体。函数遍历gpio_devices链表，符合的窗口所在的gpio_device,就是引脚的gpio_device，
gdev->desc[gpio-gdev->base]就是desc结构体。
```c
struct gpio_desc *gpio_to_desc(unsigned gpio)
{
	struct gpio_device *gdev;
	unsigned long flags;

	spin_lock_irqsave(&gpio_lock, flags);

	list_for_each_entry(gdev, &gpio_devices, list) {
		if (gdev->base <= gpio &&
		    gdev->base + gdev->ngpio > gpio) {
			spin_unlock_irqrestore(&gpio_lock, flags);
			return &gdev->descs[gpio - gdev->base];
		}
	}

	spin_unlock_irqrestore(&gpio_lock, flags);

	if (!gpio_is_valid(gpio))
		pr_warn("invalid GPIO %d\n", gpio);

	return NULL;
}
```

最终调用
desc->gdev->chip->direction_output和
desc->gdev->chip->set设置io引脚状态。

## led 子系统

		led-act {
			label = "ACT";
			default-state = "on";
			linux,default-trigger = "mmc0";
			gpios = <0x07 0x15 0x00>;
			phandle = <0x58>;
		};

led的设备树说明了它的gpio引脚和激活状态，因此可以通过gpio来设置电平。
同时它的linux,default-trigger也说明了它何时触发。
有各种触发器Documentation/devicetree/bindings/leds/common.yaml
led_classdev 是一系列的操作，驱动实现者要实现对应的函数指针。
```c
struct led_classdev {
	const char		*name;
	unsigned int brightness;
	unsigned int max_brightness;
	unsigned int color;
	int			 flags;

	/* Lower 16 bits reflect status */
#define LED_SUSPENDED		BIT(0)
#define LED_UNREGISTERING	BIT(1)
	/* Upper 16 bits reflect control information */
#define LED_CORE_SUSPENDRESUME	BIT(16)
#define LED_SYSFS_DISABLE	BIT(17)
#define LED_DEV_CAP_FLASH	BIT(18)
#define LED_HW_PLUGGABLE	BIT(19)
#define LED_PANIC_INDICATOR	BIT(20)
#define LED_BRIGHT_HW_CHANGED	BIT(21)
#define LED_RETAIN_AT_SHUTDOWN	BIT(22)
#define LED_INIT_DEFAULT_TRIGGER BIT(23)
	/* Additions for Raspberry Pi PWR LED */
#define SET_GPIO_INPUT		BIT(30)
#define SET_GPIO_OUTPUT		BIT(31)

	/* set_brightness_work / blink_timer flags, atomic, private. */
	unsigned long		work_flags;

#define LED_BLINK_SW			0
#define LED_BLINK_ONESHOT		1
#define LED_BLINK_ONESHOT_STOP		2
#define LED_BLINK_INVERT		3
#define LED_BLINK_BRIGHTNESS_CHANGE 	4
#define LED_BLINK_DISABLE		5
	/* Brightness off also disables hw-blinking so it is a separate action */
#define LED_SET_BRIGHTNESS_OFF		6
#define LED_SET_BRIGHTNESS		7
#define LED_SET_BLINK			8

	/* Set LED brightness level
	 * Must not sleep. Use brightness_set_blocking for drivers
	 * that can sleep while setting brightness.
	 */
	void		(*brightness_set)(struct led_classdev *led_cdev,
					  enum led_brightness brightness);
	/*
	 * Set LED brightness level immediately - it can block the caller for
	 * the time required for accessing a LED device register.
	 */
	int (*brightness_set_blocking)(struct led_classdev *led_cdev,
				       enum led_brightness brightness);
	/* Get LED brightness level */
	enum led_brightness (*brightness_get)(struct led_classdev *led_cdev);

	/*
	 * Activate hardware accelerated blink, delays are in milliseconds
	 * and if both are zero then a sensible default should be chosen.
	 * The call should adjust the timings in that case and if it can't
	 * match the values specified exactly.
	 * Deactivate blinking again when the brightness is set to LED_OFF
	 * via the brightness_set() callback.
	 * For led_blink_set_nosleep() the LED core assumes that blink_set
	 * implementations, of drivers which do not use brightness_set_blocking,
	 * will not sleep. Therefor if brightness_set_blocking is not set
	 * this function must not sleep!
	 */
	int		(*blink_set)(struct led_classdev *led_cdev,
				     unsigned long *delay_on,
				     unsigned long *delay_off);

	int (*pattern_set)(struct led_classdev *led_cdev,
			   struct led_pattern *pattern, u32 len, int repeat);
	int (*pattern_clear)(struct led_classdev *led_cdev);

	struct device		*dev;
	const struct attribute_group	**groups;

	struct list_head	 node;			/* LED Device list */
	const char		*default_trigger;	/* Trigger to use */

	unsigned long		 blink_delay_on, blink_delay_off;
	struct timer_list	 blink_timer;
	int			 blink_brightness;
	int			 new_blink_brightness;
	void			(*flash_resume)(struct led_classdev *led_cdev);

	struct work_struct	set_brightness_work;
	int			delayed_set_value;
	unsigned long		delayed_delay_on;
	unsigned long		delayed_delay_off;

#ifdef CONFIG_LEDS_TRIGGERS
	/* Protects the trigger data below */
	struct rw_semaphore	 trigger_lock;

	struct led_trigger	*trigger;
	struct list_head	 trig_list;
	void			*trigger_data;
	/* true if activated - deactivate routine uses it to do cleanup */
	bool			activated;

	/* LEDs that have private triggers have this set */
	struct led_hw_trigger_type	*trigger_type;

	/* Unique trigger name supported by LED set in hw control mode */
	const char		*hw_control_trigger;
	/*
	 * Check if the LED driver supports the requested mode provided by the
	 * defined supported trigger to setup the LED to hw control mode.
	 *
	 * Return 0 on success. Return -EOPNOTSUPP when the passed flags are not
	 * supported and software fallback needs to be used.
	 * Return a negative error number on any other case  for check fail due
	 * to various reason like device not ready or timeouts.
	 */
	int			(*hw_control_is_supported)(struct led_classdev *led_cdev,
							   unsigned long flags);
	/*
	 * Activate hardware control, LED driver will use the provided flags
	 * from the supported trigger and setup the LED to be driven by hardware
	 * following the requested mode from the trigger flags.
	 * Deactivate hardware blink control by setting brightness to LED_OFF via
	 * the brightness_set() callback.
	 *
	 * Return 0 on success, a negative error number on flags apply fail.
	 */
	int			(*hw_control_set)(struct led_classdev *led_cdev,
						  unsigned long flags);
	/*
	 * Get from the LED driver the current mode that the LED is set in hw
	 * control mode and put them in flags.
	 * Trigger can use this to get the initial state of a LED already set in
	 * hardware blink control.
	 *
	 * Return 0 on success, a negative error number on failing parsing the
	 * initial mode. Error from this function is NOT FATAL as the device
	 * may be in a not supported initial state by the attached LED trigger.
	 */
	int			(*hw_control_get)(struct led_classdev *led_cdev,
						  unsigned long *flags);
	/*
	 * Get the device this LED blinks in response to.
	 * e.g. for a PHY LED, it is the network device. If the LED is
	 * not yet associated to a device, return NULL.
	 */
	struct device		*(*hw_control_get_device)(struct led_classdev *led_cdev);
#endif

#ifdef CONFIG_LEDS_BRIGHTNESS_HW_CHANGED
	int			 brightness_hw_changed;
	struct kernfs_node	*brightness_hw_changed_kn;
#endif

	/* Ensures consistent access to the LED Flash Class device */
	struct mutex		led_access;
};
```

初始化ledclass_dev后用led_classdev_register_ext函数将led设备注册到led_class。
```c
		led_priv->leds[i].led.brightness_set = led_set;
		led_priv->leds[i].led.brightness_set_blocking =
			led_set_blocking;

		init_data.fwnode = &node->fwnode;
		devm_led_classdev_register_ext(dev, &led_priv->leds[i].led,
					       &init_data);
```