#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/uaccess.h>
#include <linux/types.h>

static int lab_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    const char *label = NULL;
    u32 temp_millic = 0;
    const void *prop;
    int len, i;

    dev_info(&pdev->dev, "lab_sensor: probe()\n");

    if (!np) {
        dev_err(&pdev->dev, "no device tree node\n");
        return -ENODEV;
    }

    if (of_property_read_string(np, "label", &label) == 0)
        dev_info(&pdev->dev, "  label       : %s\n", label);
    else
        dev_info(&pdev->dev, "  label       : <not set>\n");

    if (of_property_read_u32(np, "temp-millic", &temp_millic) == 0) {
        dev_info(&pdev->dev, "  temp_millic : %u\n", temp_millic);
    } else {
        dev_info(&pdev->dev, "  temp_millic : <not set>\n");
    }

    prop = of_get_property(np, "id-bytes", &len);
    if (prop && len > 0) {
        char buf[64] = {0};
        int n = min(len, (int)(sizeof(buf)-1));
        for (i = 0; i < n; i++)
            sprintf(buf + i*2, "%02x", ((const u8*)prop)[i]);
        dev_info(&pdev->dev, "  id-bytes    : %s\n", buf);
    } else {
        dev_info(&pdev->dev, "  id-bytes    : <none>\n");
    }

    return 0;
}

static int lab_remove(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "lab_sensor: removed\n");
    return 0;
}

static const struct of_device_id lab_of_match[] = {
    { .compatible = "lab,temp-sensor", },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, lab_of_match);

static struct platform_driver lab_driver = {
    .probe = lab_probe,
    .remove = lab_remove,
    .driver = {
        .name = "lab-sensor",
        .of_match_table = lab_of_match,
    },
};

module_platform_driver(lab_driver);

MODULE_AUTHOR("Prajna");
MODULE_DESCRIPTION("Lab sensor DT demo driver");
MODULE_LICENSE("GPL");
