CC = gcc
ifeq ($(OS),Windows_NT)
	CFLAGS = -std=c99 -Wall -Wextra -O2 -D__USE_MINGW_ANSI_STDIO=1 -Dalloca=__builtin_alloca -Iinclude -Iengine -Iingest -Iservice -Ithird_party
	LDFLAGS = -lws2_32
	TARGET = chronotrace.exe
	TEST_TARGET = test_all.exe
else
	CFLAGS = -std=gnu99 -Wall -Wextra -O2 -D_GNU_SOURCE -Dalloca=__builtin_alloca -DMG_ARCH=1 -Iinclude -Iengine -Iingest -Iservice -Ithird_party
	LDFLAGS = -lpthread -lm
	TARGET = chronotrace
	TEST_TARGET = test_all
endif

ENGINE_SRCS = \
	engine/core/arena.c \
	engine/core/vector.c \
	engine/core/queue.c \
	engine/core/stack.c \
	engine/core/deque.c \
	engine/core/hashmap.c \
	engine/core/lru.c \
	engine/graph/graph.c \
	engine/graph/bfs.c \
	engine/graph/dfs.c \
	engine/graph/dijkstra.c \
	engine/graph/dsu.c \
	engine/graph/tarjan.c \
	engine/order/minheap.c \
	engine/order/sort.c \
	engine/order/search.c \
	engine/order/kway.c \
	engine/order/timeline.c \
	engine/text/trie.c \
	engine/integrity/sha256.c \
	engine/integrity/merkle.c \
	engine/integrity/integrity.c \
	ingest/csv_parser.c \
	ingest/validation.c \
	ingest/storage.c \
	ingest/report.c

SERVICE_SRCS = \
	third_party/cJSON.c \
	third_party/mongoose.c \
	service/api.c \
	service/server.c \
	service/main.c

TEST_SRCS = \
	tests/test_all.c \
	tests/test_vector.c \
	tests/test_queue.c \
	tests/test_stack.c \
	tests/test_deque.c \
	tests/test_arena.c \
	tests/test_hashmap.c \
	tests/test_lru.c \
	tests/test_graph.c \
	tests/test_bfs.c \
	tests/test_dfs.c \
	tests/test_minheap.c \
	tests/test_dijkstra.c \
	tests/test_dsu.c \
	tests/test_tarjan.c \
	tests/test_sort.c \
	tests/test_search.c \
	tests/test_kway.c \
	tests/test_timeline.c \
	tests/test_trie.c \
	tests/test_sha256.c \
	tests/test_merkle.c \
	tests/test_integrity.c \
	tests/test_import.c

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(ENGINE_SRCS) $(SERVICE_SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_TARGET): $(ENGINE_SRCS) third_party/cJSON.c $(TEST_SRCS)
	$(CC) $(CFLAGS) -DTEST_ALL_RUNNER $^ -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TARGET) $(TEST_TARGET) *.o
