#!/usr/bin/env python3

import pytest
from tll.asynctll.asyncio import asyncloop_run

@asyncloop_run
async def test_continue(asyncloop, tmp_path):
    url = f'stream+pub+tcp://{tmp_path}/online.sock;request=tcp://{tmp_path}/request.sock'
    s = asyncloop.Channel(url, name='server', mode='server', storage=f'file://{tmp_path}/storage.dat')
    c = asyncloop.Channel('restore+' + url, name='client', mode='client', file=tmp_path / 'seq.dat')

    s.open()
    for i in range(10):
        s.post(b'%04d' % i, seq=i * 10)

    #assert sorted([m.name for m in c.scheme_load(c.Type.Control).messages]) == ['Ack'] # Bypass async channel scheme cache
    c.open()
    assert sorted([m.name for m in c.scheme_control.messages]) == ['Ack', 'BeginOfBlock', 'EndOfBlock', 'Online']
    assert c.config.sub('child.open').as_dict() == {'mode': 'initial'}
    assert (await c.recv_state()) == c.State.Active

    for i in range(5):
        m = await c.recv()
        assert (m.type, m.seq) == (m.Type.Data, i * 10)
        c.post(b'', name='Ack', type=c.Type.Control, seq=i * 10)
    c.close()
    c.open()

    assert c.config.sub('child.open').as_dict() == {'mode': 'seq', 'seq': '41'}
    m = await c.recv()
    assert (m.type, m.seq) == (m.Type.Data, 50)

    c.close()
    c.open()

    assert c.config.sub('child.open').as_dict() == {'mode': 'seq', 'seq': '41'}
