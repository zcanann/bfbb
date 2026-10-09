"""Original-backed checks for the group-to-event proof dependency."""
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from platforms.verify_reviewed import Original, REFERENCES, TARGET
from platforms.france_group_sequence import generate_unit as generate_group
from platforms.france_event_sequence import generate_unit as generate_event


@unittest.skipUnless(os.environ.get('BFBB_FRANCE_TEST_ORIG'),
                     'Set BFBB_FRANCE_TEST_ORIG for original fixtures')
class EventOriginalTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        records = json.loads((ROOT / 'config/platforms/versions.json').read_text())['versions']
        cls.originals = {v: Original(v, records[v], Path(os.environ['BFBB_FRANCE_TEST_ORIG']))
                         for v in (*REFERENCES, TARGET)}
        for version, original in cls.originals.items():
            layout = json.loads((ROOT / 'config/platforms' / version / 'region-layout.json').read_text())
            if layout['executable_sha1'] != original.sha1:
                raise ValueError('Original layout identity differs')
            original._stream_regions = {r['name']: r for r in layout['regions']}
        cls.registry = ROOT / 'config/platforms/SLES-53623'
        cls.group = generate_group(cls.originals, cls.registry)
        cls.event = generate_event(cls.originals, cls.registry)

    def test_event_identities_cannot_become_their_own_group_anchor(self):
        with tempfile.TemporaryDirectory() as temporary:
            registry = Path(temporary)
            for path in self.registry.glob('*.json'):
                shutil.copyfile(path, registry / path.name)
            path = registry / 'tu-corroborated-functions.json'
            document = json.loads(path.read_text())
            document['functions'] = [f for f in document['functions']
                                     if f['source'] != 'SB/Core/x/xEvent.cpp']
            for record in self.event['functions']:
                document['functions'].append({**record, 'sha256': '0' * 64,
                                               'boundary_confirmation': False})
            path.write_text(json.dumps(document), encoding='utf-8')
            # Regeneration ignores downstream claims and reconstructs the same
            # evidence. Whole-registry ingestion separately rejects stale claims.
            self.assertEqual(generate_group(self.originals, registry), self.group)
            self.assertEqual(generate_event(self.originals, registry), self.event)

    def test_original_event_body_change_is_rejected(self):
        target = self.originals[TARGET]
        entry = next(f for f in self.event['functions'] if f['size'] == 412)['address']
        segment = next(s for s in target.loaded
                       if s['address'] <= entry < s['address'] + s['file_size'])
        offset = segment['offset'] + entry - segment['address']
        saved = target.data
        altered = bytearray(saved)
        altered[offset] ^= 1
        target.data = bytes(altered)
        try:
            with self.assertRaises(ValueError):
                generate_event(self.originals, self.registry)
        finally:
            target.data = saved


if __name__ == '__main__':
    unittest.main()
