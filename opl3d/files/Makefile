APP = opl3d

# Add any other object files to this list below
APP_OBJS = opl3d.o

all: build install

build: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS)

install: $(APP)
	$(TARGETINST) -d $(APP) /bin/$(APP)
	$(TARGETINST) -d -p 0755 $(APP).sh /etc/init.d/$(APP)

clean: 
	-rm -f $(APP) *.elf *.gdb *.o

