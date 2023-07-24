DATE=$(shell date +%F)
USER=slechev
IMAGE=openwebrxplus-softmbe

all:
	docker buildx create --name owrxp-builder --driver docker-container --bootstrap --use --driver-opt network=host || true
	docker buildx build --push --pull --platform=linux/amd64,linux/arm64,linux/arm/v7 -t $(USER)/$(IMAGE):$(DATE) -t $(USER)/$(IMAGE) .
	docker buildx rm --keep-state owrxp-builder

run:
	@docker run --rm -h $(IMAGE) --name $(IMAGE) --device /dev/bus/usb -p 8073:8073 -v openwebrxplus-settings:/var/lib/openwebrx $(USER)/$(IMAGE)

admin:
	@docker exec -it $(USER)/$(IMAGE) /usr/bin/openwebrx admin adduser af

push:
	@docker push $(USER)/$(IMAGE):$(DATE)
	@docker push $(USER)/$(IMAGE)

dev:
	@S6_CMD_ARG0=/bin/bash docker run -it --rm -p 8073:8073 --device /dev/bus/usb --name owrxp-build --entrypoint /bin/bash $(USER)/$(IMAGE)
