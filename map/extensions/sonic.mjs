// import "@mapeditor/tiled-api";

class BinaryWriter {
    constructor(buf, littleEndian = true) {
        this.view = new DataView(buf);
        this.cursor = 0;
        this.littleEndian = littleEndian;
    }

    u8(value) {
        this.view.setUint8(this.cursor, value, this.littleEndian);
        this.cursor += 1;
    }

    u16(value) {
        this.view.setUint16(this.cursor, value, this.littleEndian);
        this.cursor += 2;
    }

    u32(value) {
        this.view.setUint32(this.cursor, value, this.littleEndian);
        this.cursor += 4;
    }

    i8(value) {
        this.view.setInt8(this.cursor, value, this.littleEndian);
        this.cursor += 1;
    }

    i16(value) {
        this.view.setInt16(this.cursor, value, this.littleEndian);
        this.cursor += 2;
    }

    i32(value) {
        this.view.setInt32(this.cursor, value, this.littleEndian);
        this.cursor += 4;
    }

    bool(value) {
        this.view.setUint8(this.cursor, value ? 1 : 0, this.littleEndian);
        this.cursor += 1;
    }

    /**
     * @param {string} value
     * @param {number | undefined} size
     */
    cString(value, size = undefined) {
        for (let i = 0; i < value.length; i += 1)
            this.view.setUint8(this.cursor + i, value.charCodeAt(i), this.littleEndian);
        this.view.setUint8(this.cursor + value.length, 0, this.littleEndian);

        if (size != undefined) {
            this.cursor += size
        } else {
            this.cursor += value.length + 1;
        }
    }

    skip(count) {
        this.cursor += count;
    }

    seek(offset) {
        this.cursor = offset;
    }

    rewind() {
        this.cursor = 0;
    }

    slice(size) {
        const s = new BinaryWriter(null, this.littleEndian);
        s.cursor = this.cursor;
        s.view = this.view;
        this.cursor += size;
        return s;
    }
}

function assert(c) {
    if (!c) throw new Error("Assertion Failed");
}

/** @type ScriptedMapFormat */
const sonicFileFormat = {
    name: "Sonic stage binary format",
    extension: "stage",

    write: function(map, fileName) {
        /** @type TileLayer */
        const foreground = map.layers.find((l) => l.name == "foreground");
        assert(foreground.isTileLayer);
        /** @type TileLayer */
        const collision = map.layers.find((l) => l.name == "collision");
        assert(collision.isTileLayer);
        /** @type ObjectGroup */
        const objects = map.layers.find((l) => l.name == "objects");
        assert(objects.isObjectLayer);

        const data = new ArrayBuffer(
            8                                         // width and height of the stage
            + (map.width * map.height * 10)           // visual data
            + (map.width * map.height * 10)           // collision data
            + 4                                       // object count
            + (objects.objectCount * (64 + 8 + 1024)) // object data
        );
        const writer = new BinaryWriter(data);

        writer.u32(map.width);
        writer.u32(map.height);

        for (let x = 0; x < map.width; x += 1) {
            for (let y = 0; y < map.height; y += 1) {
                const tile = foreground.cellAt(x, y);
                assert(tile.tileId >= -1)
                if (tile.empty) {
                    writer.i32(-1);
                    writer.i32(-1);
                    writer.bool(0);
                    writer.bool(0);
                } else {
                    writer.i32(tile.tileId % 32);
                    writer.i32(Math.floor(tile.tileId / 32));
                    writer.bool(tile.flippedHorizontally);
                    writer.bool(tile.flippedVertically);
                }
            }
        }

        for (let x = 0; x < map.width; x += 1) {
            for (let y = 0; y < map.height; y += 1) {
                const tile = collision.cellAt(x, y);
                assert(tile.tileId >= -1)
                if (tile.empty) {
                    writer.i32(-1);
                    writer.i32(-1);
                    writer.bool(0);
                    writer.bool(0);
                } else {
                    writer.i32(tile.tileId % 32);
                    writer.i32(Math.floor(tile.tileId / 32));
                    writer.bool(tile.flippedHorizontally);
                    writer.bool(tile.flippedVertically);
                }
            }
        }

        writer.u32(objects.objectCount);

        for (let i = 0; i < objects.objectCount; i += 1) {
            const object = objects.objectAt(i);

            writer.cString(object.className, 64);
            writer.i32(Math.round(object.x));
            writer.i32(Math.round(object.y));
            writer.skip(1024);
            // TODO: userdata
        }

        const file = new BinaryFile(fileName, BinaryFile.WriteOnly);
        file.write(data);
        file.commit();
    },
}

tiled.registerMapFormat("sonic", sonicFileFormat);
