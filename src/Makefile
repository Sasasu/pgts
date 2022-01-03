MODULE_big = pgts
EXTENSION = pgts
DATA = pgts--0.1.0.sql

FILES += $(shell find . -name '*.c' -type f)
OBJS += $(foreach FILE, $(FILES), $(subst .c,.o, $(FILE)))

SHLIB_LINK += -lzstd

REGRESS += it_works
REGRESS_OPTS += --outputdir=../tests \
				--inputdir=../tests \
				--use-existing

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
