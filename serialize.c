#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct test
{
    int a;
    int b;
    char c[10];
};

void serialize(char *filename, void *buf, int bufsz)
{

    // calculate that aligned size
    int aligned_size = (bufsz > sizeof(uint32_t))
                           ? bufsz + (bufsz % sizeof(uint32_t))
                           : sizeof(uint32_t);

    // allocate uint32_t aligned buffer
    char uint_aligned_buf[aligned_size];

    // zero out the extra bytes at the end. if any
    memset(uint_aligned_buf, 0, aligned_size);

    // copy into buffer
    memcpy(uint_aligned_buf, buf, bufsz);

    // cast to uint32_t array
    uint32_t *uint_pack = (uint32_t *)&(uint_aligned_buf[0]);

    int uint_pack_sz = aligned_size / sizeof(uint32_t);

    // allocate buffer for data stored in network byte order.
    uint32_t net_byte_order[uint_pack_sz];

    for (int i = 0; i < uint_pack_sz; i++)
    {
        net_byte_order[i] = htonl(uint_pack[i]);
    }

    int fd = open(filename, O_CREAT | O_WRONLY, 0666);

    // convert sizes to net byte ord.
    aligned_size = htonl((uint32_t)aligned_size);
    bufsz = htonl((uint32_t)bufsz);

    // write the sizes to the file
    write(fd, &aligned_size, sizeof(uint32_t));
    write(fd, &bufsz, sizeof(uint32_t));

    // write the data.
    write(fd, &(net_byte_order[0]), (int) ntohl(aligned_size));

    // close file
    close(fd);
}

int deserialize(char *filename, void *buf, int bufsz)
{
    int fd = open(filename, O_RDONLY);
    if (fd < -1)
        return 0;

    // read the data size stored
    int data_sz;
    if (read(fd, &data_sz, sizeof(uint32_t)) != sizeof(uint32_t))
        return 0;

    data_sz = (int)ntohl(data_sz);

    // read the actual size before alignment
    int proper_sz;
    if (read(fd, &proper_sz, sizeof(uint32_t)) != sizeof(uint32_t))
        return 0;

    proper_sz = (int)ntohl((uint32_t)proper_sz);

    // if proper size is greater than data size, something went wrong.
    if (proper_sz > data_sz)
        return 0;

    // allocate two buffers to store the original data.
    uint32_t raw_data[data_sz];
    uint32_t converted[data_sz];

    // we should be able to store all data in buffer
    if (data_sz <= bufsz)
    {
        int bytes_read;
        // read the data
        if ((bytes_read = read(fd, &(raw_data[0]), data_sz)) == (ssize_t)data_sz)
        {

            for (int i = 0; i < data_sz / sizeof(uint32_t); i++)
            {
                converted[i] = ntohl(raw_data[i]);
            }
        } else {
            return 0;
        }
    } else {
        return 0;
    }

    memcpy(buf, converted, proper_sz);

    return 1;
}

int main(int argc, char **argv)
{

    struct test a;
    a.a = 1;
    a.b = 23;
    strcpy(a.c, "Hello");

    serialize("data.ser", &a, sizeof(struct test));

    struct test b;

    if (deserialize("data.ser", &b, sizeof(struct test))){
        printf("b.a = %d", b.a);
        printf("b.b = %d", b.b);
        printf("b.c = %s", b.c);
    }

}