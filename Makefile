DATE=$(shell date +%F)
USER=slechev
IMAGE=openwebrxplus-softmbe
ARCH=$(shell uname -m)

all:
	#docker build --platform=linux/arm64 --build-arg ARCH=$(ARCH) -t $(USER)/$(IMAGE):$(DATE)-arm64 -t $(USER)/$(IMAGE):$(DATE) -t $(USER)/$(IMAGE) .
	docker build --platform=linux/amd64 --build-arg ARCH=$(ARCH) -t $(USER)/$(IMAGE):$(DATE)-amd64 -t $(USER)/$(IMAGE):$(DATE) -t $(USER)/$(IMAGE) .

run:
	@docker run --rm -h $(IMAGE) --name $(IMAGE) --device /dev/bus/usb -p 8073:8073 -v openwebrxplus-settings:/var/lib/openwebrx $(USER)/$(IMAGE)

admin:
	@docker exec -it $(USER)/$(IMAGE) /usr/bin/openwebrx admin adduser af

push:
	@docker push $(USER)/$(IMAGE):$(DATE)
	@docker push $(USER)/$(IMAGE)

dev:
	@S6_CMD_ARG0=/bin/bash docker run -it --rm -p 8073:8073 --device /dev/bus/usb --name owrxp-build --entrypoint /bin/bash $(USER)/$(IMAGE)
