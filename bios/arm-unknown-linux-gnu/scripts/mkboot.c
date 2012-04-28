#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	char	sector[512];
	int	r, fd, sect;

	if (argc < 3) {
		fprintf(stderr, "Usage: %s DEVICE OFFSET\n", argv[0]);
		exit(1);
	}

	sect = strtoul(argv[2], NULL, 10);

	fd = open(argv[1], O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "%s: unable to open %s\n", argv[0], argv[1]);
		exit(1);
	}

	memset(sector, 0, sizeof(sector));

	sector[510] = 'R';
	sector[511] = 'K';

	sector[0x1be] = 0x80;
	sector[0x1c6] = sect;
	sector[0x1c7] = sect >> 8;
	sector[0x1c8] = sect >> 16;
	sector[0x1c9] = sect >> 24;

	write(fd, sector, sizeof(sector));

	close(fd);

	return 0;
}
