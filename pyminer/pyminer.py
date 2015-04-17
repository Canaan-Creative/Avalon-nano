#!/usr/bin/python
#
# Copyright 2015 Mikeqin Fengling.Qin@gmail.com
# Copyright 2011 Jeff Garzik
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#

import time
import json
import pprint
import hashlib
import struct
import re
import base64
import httplib
import sys
import usb.core
import usb.util
import binascii
from multiprocessing import Process
from midstate import calculateMidstate

ERR_SLEEP = 15

TYPE_WORK = "13"

settings = {}
pp = pprint.PrettyPrinter(indent=4)

class BitcoinRPC:
	OBJID = 1

	def __init__(self, host, port, username, password):
		authpair = "%s:%s" % (username, password)
		self.authhdr = "Basic %s" % (base64.b64encode(authpair))
		self.conn = httplib.HTTPConnection(host, port, False, 30)
	def rpc(self, method, params=None):
		self.OBJID += 1
		obj = { 'version' : '1.1',
			'method' : method,
			'id' : self.OBJID }
		if params is None:
			obj['params'] = []
		else:
			obj['params'] = params
		self.conn.request('POST', '/', json.dumps(obj),
			{ 'Authorization' : self.authhdr,
			  'Content-type' : 'application/json' })

		resp = self.conn.getresponse()
		if resp is None:
			print "JSON-RPC: no response"
			return None

		body = resp.read()
		resp_obj = json.loads(body)
		if resp_obj is None:
			print "JSON-RPC: cannot JSON-decode body"
			return None
		if 'error' in resp_obj and resp_obj['error'] != None:
			return resp_obj['error']
		if 'result' not in resp_obj:
			print "JSON-RPC: no result in object"
			return None

		return resp_obj['result']
	def getblockcount(self):
		return self.rpc('getblockcount')
	def getwork(self, data=None):
		return self.rpc('getwork', data)

def uint32(x):
	return x & 0xffffffffL

def bytereverse(x):
	return uint32(( ((x) << 24) | (((x) << 8) & 0x00ff0000) |
			(((x) >> 8) & 0x0000ff00) | ((x) >> 24) ))

def bufreverse(in_buf):
	out_words = []
	for i in range(0, len(in_buf), 4):
		word = struct.unpack('@I', in_buf[i:i+4])[0]
		out_words.append(struct.pack('@I', bytereverse(word)))
	return ''.join(out_words)

def wordreverse(in_buf):
	out_words = []
	for i in range(0, len(in_buf), 4):
		out_words.append(in_buf[i:i+4])
	out_words.reverse()
	return ''.join(out_words)

class Miner:
        usbdev = None
        endpin = None
        endpout = None
	def __init__(self, id, vendor_id, product_id):
		self.id = id
                self.usbdev, self.endpin, self.endpout = self.enum_usbdev(vendor_id, product_id)

        def enum_usbdev(self, vendor_id, product_id):
            # Find device
            usbdev = usb.core.find(idVendor = vendor_id, idProduct = product_id)

            if not usbdev:
                sys.exit("Avalon nano cann't be found!")
            else:
                print "Find an Avalon nano"

            try:
                if usbdev.is_kernel_driver_active(0) is True:
                    usbdev.detach_kernel_driver(0)
            except:
                print "detach kernel driver failed"

            try:
                # usbdev[iConfiguration][(bInterfaceNumber,bAlternateSetting)]
                for endp in usbdev[0][(0,0)]:
                    if endp.bEndpointAddress & 0x80:
                        endpin = endp.bEndpointAddress
                    else:
                        endpout = endp.bEndpointAddress

            except usb.core.USBError as e:
                sys.exit("Could not set configuration: %s" % str(e))

            return usbdev, endpin, endpout

        def CRC16(self, message):
                #CRC-16-CITT poly, the CRC sheme used by ymodem protocol
                poly = 0x1021
                 #16bit operation register, initialized to zeros
                reg = 0x0000
                #pad the end of the message with the size of the poly
                message += '\x00\x00'
                #for each bit in the message
                for byte in message:
                        mask = 0x80
                        while(mask > 0):
                                #left shift by one
                                reg<<=1
                                #input the next bit from the message into the right hand side of the op reg
                                if ord(byte) & mask:
                                        reg += 1
                                mask>>=1
                                #if a one popped out the left of the reg, xor reg w/poly
                                if reg > 0xffff:
                                        #eliminate any one that popped out the left
                                        reg &= 0xffff
                                        #xor with the poly, this is the remainder
                                        reg ^= poly
                return reg

        def mm_package(self, cmd_type, idx = "01", cnt = "01", module_id = None, pdata = '0'):
                if module_id == None:
                    data = pdata.ljust(64, '0')
                else:
                    data = pdata.ljust(60, '0') + module_id.rjust(4, '0')
                crc = self.CRC16(data.decode("hex"))
                print "434e" + cmd_type + "00" + idx + cnt + data + hex(crc)[2:].rjust(4, '0')
                return "434e" + cmd_type + "00" + idx + cnt + data + hex(crc)[2:].rjust(4, '0')

	def work(self, datastr, targetstr):
		# decode work data hex string to binary
		static_data = datastr.decode('hex')
		static_data = bufreverse(static_data)

		# the first 76b of 80b do not change
		blk_hdr = static_data[:76]

		# decode 256-bit target value
		targetbin = targetstr.decode('hex')
		targetbin = targetbin[::-1]	# byte-swap and dword-swap
		targetbin_str = targetbin.encode('hex')
		target = long(targetbin_str, 16)

		# pre-hash first 76b of block header
		static_hash = hashlib.sha256()
		static_hash.update(blk_hdr)

                # calculate midstate
                midstate_bin = calculateMidstate(datastr.decode('hex')[:64])

                # send task to Avalon nano
                icarus_bin = midstate_bin[::-1] + '0'.rjust(40, '0').decode('hex') + datastr.decode('hex')[64:76][::-1]
                if settings['verbose'] == 1:
                        print 'send task:' + icarus_bin.encode('hex')
                workpkg = icarus_bin.encode('hex')[:64]
                work_bin = self.mm_package(TYPE_WORK, "01", "02", pdata = workpkg).decode('hex')
                self.usbdev.write(self.endpout, work_bin)
                workpkg = icarus_bin.encode('hex')[64:128]
                work_bin = self.mm_package(TYPE_WORK, "02", "02", pdata = workpkg).decode('hex')
                self.usbdev.write(self.endpout, work_bin)

                # read nonce back
                rdata = None
                loop = 0
                while (rdata == None):
                    try:
                        rdata = self.usbdev.read(self.endpin, 40)
                    except:
                        pass

                    time.sleep(0.01)
                    loop = loop + 1
                    if loop == 3:
                        break

                if rdata == None or rdata[2] != 0x23:
                        print time.asctime(), "No Nonce found"
                        return (0xffffffff, None)
                else:
                        if settings['verbose'] == 1:
                            print 'nonce:', binascii.hexlify(rdata)[12:20]

                # encode 32-bit nonce value
                nonce = (rdata[6] << 24) | (rdata[7] << 16) | (rdata[8] << 8) | rdata[9]
                nonce = bytereverse(nonce)
                nonce_bin = struct.pack("<I", nonce)

                # hash final 4b, the nonce value
                hash1_o = static_hash.copy()
                hash1_o.update(nonce_bin)
                hash1 = hash1_o.digest()

                # sha256 hash of sha256 hash
                hash_o = hashlib.sha256()
                hash_o.update(hash1)
                hash = hash_o.digest()

                # quick test for winning solution: high 32 bits zero?
                if hash[-4:] != '\0\0\0\0':
                        print time.asctime(), "Invalid Nonce"
                        return (0xffffffff, None)

                # convert binary hash to 256-bit Python long
                hash = bufreverse(hash)
                hash = wordreverse(hash)

                hash_str = hash.encode('hex')
                l = long(hash_str, 16)

                # proof-of-work test:  hash < target
                if l < target:
                        print time.asctime(), "PROOF-OF-WORK found: %064x" % (l,)
                        return (0xffffffff, nonce_bin)
                else:
                        print time.asctime(), "PROOF-OF-WORK false positive %064x" % (l,)
		return (0xffffffff, None)

	def submit_work(self, rpc, original_data, nonce_bin):
		nonce_bin = bufreverse(nonce_bin)
		nonce = nonce_bin.encode('hex')
		solution = original_data[:152] + nonce + original_data[160:256]
		param_arr = [ solution ]
		result = rpc.getwork(param_arr)
		print time.asctime(), "--> Upstream RPC result:", result

	def iterate(self, rpc):
		work = rpc.getwork()
		if work is None:
			time.sleep(ERR_SLEEP)
			return
		if 'data' not in work or 'target' not in work:
			time.sleep(ERR_SLEEP)
			return

		time_start = time.time()

                (hashes_done, nonce_bin) = self.work(work['data'],
                                                      work['target'])

		time_end = time.time()
		time_diff = time_end - time_start

		if settings['hashmeter']:
			print "HashMeter(%d): %d hashes, %.2f Ghash/sec" % (
			      self.id, 0xffffffff,
			      (0xffffffff / 1000000000.0) / time_diff)

		if nonce_bin is not None:
			self.submit_work(rpc, work['data'], nonce_bin)

	def loop(self):
		rpc = BitcoinRPC(settings['host'], settings['port'],
				 settings['rpcuser'], settings['rpcpass'])
		if rpc is None:
			return

		while True:
			self.iterate(rpc)

def miner_thread(id, vendor_id, product_id):
	miner = Miner(id, vendor_id, product_id)
	miner.loop()

if __name__ == '__main__':
	if len(sys.argv) != 2:
		print "Usage: pyminer.py CONFIG-FILE"
		sys.exit(1)

	f = open(sys.argv[1])
	for line in f:
		# skip comment lines
		m = re.search('^\s*#', line)
		if m:
			continue

		# parse key=value lines
		m = re.search('^(\w+)\s*=\s*(\S.*)$', line)
		if m is None:
			continue
		settings[m.group(1)] = m.group(2)
	f.close()

	if 'host' not in settings:
		settings['host'] = '127.0.0.1'
	if 'port' not in settings:
		settings['port'] = 8332
	if 'threads' not in settings:
		settings['threads'] = 1
	if 'hashmeter' not in settings:
		settings['hashmeter'] = 0
	if 'scantime' not in settings:
		settings['scantime'] = 30L
        if 'tty' not in settings:
                settings['tty'] = '/dev/ttyACM0'
	if 'rpcuser' not in settings or 'rpcpass' not in settings:
		print "Missing username and/or password in cfg file"
		sys.exit(1)
        if 'verbose' not in settings:
                settings['verbose'] = 0

	settings['port'] = int(settings['port'])
        # TODO: Support multithread
	settings['threads'] = 1;
	settings['hashmeter'] = int(settings['hashmeter'])
	settings['scantime'] = long(settings['scantime'])
        settings['verbose'] = int(settings['verbose'])

	thr_list = []
        nano_vid = 0x29f1
        nano_pid = 0x33f1
	for thr_id in range(settings['threads']):
		p = Process(target=miner_thread, args=(thr_id, nano_vid, nano_pid))
		p.start()
		thr_list.append(p)
		time.sleep(1)			# stagger threads

	print settings['threads'], "mining threads started"

	print time.asctime(), "Miner Starts - %s:%s" % (settings['host'], settings['port'])
	try:
		for thr_proc in thr_list:
			thr_proc.join()
	except KeyboardInterrupt:
		pass
	print time.asctime(), "Miner Stops - %s:%s" % (settings['host'], settings['port'])

