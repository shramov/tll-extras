#!/usr/bin/env python3
# vim: sts=4 sw=4 et

import pytest

import os

from tll.asynctll import asyncio as asynctll
from tll.channel import Context
import tll.logger

tll.logger.init()

@pytest.fixture
def anyio_backend():
    return 'asyncio'

@pytest.fixture
def context():
    ctx = Context()
    ctx.load(os.path.join(os.environ.get("BUILD_DIR", "build"), "tll-restore"))
    return ctx

@pytest.fixture
def asyncloop(context):
    loop = asynctll.Loop(context)
    yield loop
    loop.destroy()
