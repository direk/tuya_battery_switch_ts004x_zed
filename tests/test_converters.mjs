/** Load converters with real Z2M dependencies, register custom clusters, and
 * verify metadata used by the Binding tab. Run:
 * node tests/test_converters.mjs [/path/to/directory/with/node_modules]
 */
import assert from 'node:assert/strict';
import {mkdtemp, readFile, writeFile, symlink, rm} from 'node:fs/promises';
import {createRequire} from 'node:module';
import {dirname, join, resolve} from 'node:path';
import {tmpdir} from 'node:os';
import {fileURLToPath, pathToFileURL} from 'node:url';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const require = createRequire(join(resolve(process.argv[2] ?? root), 'package.json'));
const modules = dirname(dirname(dirname(require.resolve('zigbee-herdsman-converters'))));
const {Device} = require(join(dirname(require.resolve('zigbee-herdsman')), 'controller/model/device.js'));
const temp = await mkdtemp(join(tmpdir(), 'ts004x-converters-'));
try {
    await symlink(modules, join(temp, 'node_modules'), 'dir');
    for (const n of [1, 2, 3, 4, 6]) {
        const filename = `TS004${n}-z-SlD`;
        const source = await readFile(join(root, 'zigbee2mqtt', `${filename}.js`), 'utf8');
        const path = join(temp, `${filename}.mjs`);
        await writeFile(path, source);
        const {default: definition} = await import(pathToFileURL(path).href);
        assert.equal(definition.model, filename);
        const exposes = definition.extend.flatMap(e => e.exposes ?? []);
        for (const name of ['switch_type', 'switch_actions', 'min_level', 'max_level', 'scene_id', 'group_id']) {
            const entries = exposes.filter(e => e.name === name);
            assert.equal(entries.length, n, `${filename}: ${name} count`);
            assert.equal(new Set(entries.map(e => e.endpoint)).size, n);
        }
        for (const selector of exposes.filter(e => e.name === 'switch_type')) {
            for (const mode of ['brightness_level_no_onoff', 'brightness_level_up_no_onoff', 'brightness_level_down_no_onoff']) {
                assert(selector.values.includes(mode), `${filename}: missing ${mode}`);
            }
        }
        // Exercise actual herdsman registration, not just module import. A missing
        // cluster.name reaches the UI as undefined and breaks localeCompare.
        const device = Object.create(Device.prototype);
        device._customClusters = {};
        let registered = 0;
        for (const extension of definition.extend) {
            for (const configure of extension.configure ?? []) {
                if (configure.toString().includes('device.addCustomCluster(')) {
                    await configure(device);
                    registered++;
                }
            }
        }
        assert.equal(registered, 2);
        for (const [name, cluster] of Object.entries(device.customClusters)) {
            assert.equal(cluster.name, name, `${filename}: cluster name`);
            for (const [name, attribute] of Object.entries(cluster.attributes)) {
                assert.equal(attribute.name, name, `${filename}: attribute name`);
            }
        }
        const clusters = Object.values(device.customClusters);
        clusters.sort((a, b) => a.name.localeCompare(b.name));
        assert.deepEqual(clusters.map(c => c.name), ['genOnOffSwitchCfg', 'genScenes']);
        console.log(`PASS: ${filename} mode selectors, real cluster registration, and Binding name sorting`);
    }
} finally {
    await rm(temp, {recursive: true, force: true});
}
