
SRC=src
OBJDIR=obj
HEADERS=$(SRC)/tbwatch.h
OBJECTS=$(OBJDIR)/notif.o $(OBJDIR)/tbwatch.o

PGM_DIR=/usr/sbin
PGM_LOCAL_DIR=usr/sbin
PGM_FILE_BASENAME=tbwatch
PGM_FILE=usr/sbin/$(PGM_FILE_BASENAME)

CONFIG_DIR=/etc
CONFIG_FILE_BASENAME=tbwatchpaths.cfg
CONFIG_FILE=etc/$(CONFIG_FILE_BASENAME)

SERVICE_DIR=/lib/systemd/system
SERVICE_FILE_BASENAME=tbwatch.service
SERVICE_FILE=lib/systemd/system/$(SERVICE_FILE_BASENAME)


CFLAGS=-Wall -std=c++11
LFLAGS=


all: setup tbwatch

tbwatch: $(OBJECTS)
	g++ $(LFLAGS) $(OBJECTS) -o $(PGM_FILE)

$(OBJDIR)/tbwatch.o: $(SRC)/tbwatch.cpp  $(HEADERS)
	g++ $(CFLAGS) -c $(SRC)/tbwatch.cpp -o $(OBJDIR)/tbwatch.o

$(OBJDIR)/notif.o: $(SRC)/notif.cpp $(HEADERS) 
	g++ $(CFLAGS) -c $(SRC)/notif.cpp -o $(OBJDIR)/notif.o

setup:
	mkdir -p $(OBJDIR)
	mkdir -p $(PGM_LOCAL_DIR)

.PHONY: clean install
clean:	
	rm -rf $(OBJDIR) $(PGM_LOCAL_DIR)/$(PGM_FILE_BASENAME)

install: all
	@echo "removing existing $(PGM_DIR)/$(PGM_FILE_BASENAME)"
	rm -f $(PGM_DIR)/$(PGM_FILE_BASENAME)
	@echo "copying $(PGM_LOCAL_DIR)/$(PGM_FILE_BASENAME) to $(PGM_DIR)/$(PGM_FILE_BASENAME)"
	cp $(PGM_LOCAL_DIR)/$(PGM_FILE_BASENAME) $(PGM_DIR)
	@echo "removing $(SERVICE_DIR)/$(SERVICE_FILE_BASENAME)"
	rm -f $(SERVICE_DIR)/$(SERVICE_FILE_BASENAME)
	cp $(SERVICE_FILE) $(SERVICE_DIR) 
	systemctl daemon-reload
	systemctl enable $(SERVICE_FILE_BASENAME)
	systemctl start $(SERVICE_FILE_BASENAME)

uninstall:
	@echo "removing existing $(PGM_DIR)/$(PGM_FILE_BASENAME)"
	rm -f $(PGM_DIR)/$(PGM_FILE_BASENAME)
	@echo "removing $(SERVICE_DIR)/$(SERVICE_FILE_BASENAME)"
	rm -f $(SERVICE_DIR)/$(SERVICE_FILE_BASENAME)
	systemctl daemon-reload
	systemctl stop $(SERVICE_FILE_BASENAME)
	systemctl disable $(SERVICE_FILE_BASENAME)


