#include <stdlib.h>
#include <bios/boot.h>
#include <bios/time.h>
#include <bios/config.h>
#include <bios/bootdev.h>
#include <bios/netdev.h>

extern struct netdev net_dev_3com_3c59x;
extern struct netdev net_dev_dec_21041;

static struct netdev *netdevs[] = {
#ifdef CONFIG_NET_3COM_3C59X
	&net_dev_3com_3c59x,
#endif
#ifdef CONFIG_NET_DEC_21041
	&net_dev_dec_21041,
#endif
	NULL
};

struct netdev *probed_net_devs;

static int net_probe(void)
{
	struct netdev **nd = &probed_net_devs;
	int i;

	for (i = 0; netdevs[i]; i++) {
		if (netdevs[i]->hard->probe() == 0) {
			*nd = netdevs[i];
			nd = &netdevs[i]->next;
		}
	}

	*nd = NULL;

	return probed_net_devs ? 0 : -1;
}

static int net_start(void)
{
	struct netdev *nd;

	if (!probed_net_devs)
		return -1;

	for (nd = probed_net_devs; nd; nd = nd->next)
		if (nd->hard->open(nd) == 0)
			nd->up = 1;

	if (do_bootp() != 0) {
		for (nd = probed_net_devs; nd; nd = nd->next)
			if (nd->up)
				nd->hard->close(nd);
		return -1;
	}

	return 0;
}

static int net_load(void)
{
	int ret;

	ret = do_tftp();

	if (((*(u8 *)0x40012000) & 0x20)) {
		root_dev = 0x00ff;
		root_flags = 0;
	} else {
		root_dev = 0x0303;
		root_flags = 1;
	}

	return ret;
}

static int net_stop(void)
{
	struct netdev *nd;

	for (nd = probed_net_devs; nd; nd = nd->next)
		if (nd->up)
			nd->hard->close(nd);
	return 0;
}

struct bootdev boot_net = {
	"net",

	net_probe,
	net_start,
	net_load,
	net_stop
};
