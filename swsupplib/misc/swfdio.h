#ifndef SWFDIO_H_
#define SWFDIO_H_

void swfdio_init(void);
void swfdio_set(int index, int fd);
int swfdio_get(int index);

#endif
