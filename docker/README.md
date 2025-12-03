# Docker commands

## Building the image

```bash
sudo mount -t iso9660 -o ro,exec,loop cpu2017-1.1.9.iso ./spec
docker build -f docker/SPEC.Dockerfile . --build-arg SPEC_ISO_MNT=./spec
```

## Detecting all size class distributions

For `train`:

```bash
docker run -it --rm \
    -v $PWD/docker/detector-data/train:/cpu2017/detector-data \
    $(docker build -q -f docker/SPEC.Dockerfile . --build-arg SPEC_ISO_MNT=./spec)

[docker] runcpu --action=run --size=train --config=detector $INTSPEED $FPSPEED $INTRATE $FPRATE
```

## Running to test out things

```bash
docker run -it --rm \
    -v $PWD/docker/detector-data/train:/cpu2017/detector-data \
    $(docker build -q -f docker/SPEC.Dockerfile . --build-arg SPEC_ISO_MNT=./spec)
```

## Running with littering

```bash
docker run -it --rm \
    -v $PWD/docker/detector-data/train:/cpu2017/detector-data \
    -v $PWD/litterer-log/1-0.4:/cpu2017/litterer-log \
    $(docker build -q -f docker/SPEC.Dockerfile . --build-arg SPEC_ISO_MNT=./spec)

[docker] runcpu --action=run --size=train --config=litterer -n 11 \
    --define litter_multiplier=1 \
    --define litter_occupancy=0.4 \
    $INTSPEED $FPSPEED
```
