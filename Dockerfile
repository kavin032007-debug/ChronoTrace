# Multi-stage build for ChronoTrace
FROM alpine:3.19 AS builder

RUN apk add --no-cache gcc musl-dev make

WORKDIR /app
COPY . .

# Build the Linux binary
RUN make all

# Final minimal deployment image
FROM alpine:3.19

RUN apk add --no-cache ca-certificates

WORKDIR /app
COPY --from=builder /app/chronotrace /app/chronotrace
COPY --from=builder /app/frontend /app/frontend
COPY --from=builder /app/data /app/data

EXPOSE 8080

CMD ["/app/chronotrace", "--port", "8080"]
