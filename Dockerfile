FROM alpine:latest AS builder

RUN apk add --no-cache g++ make 

WORKDIR /app 

COPY . . 

RUN make docker

FROM scratch AS base

COPY --from=builder /app/server /server
COPY --from=builder /app/index.html /index.html

CMD ["./server"]

LABEL org.opencontainers.image.source=https://github.com/jesuscbm/posthaste
