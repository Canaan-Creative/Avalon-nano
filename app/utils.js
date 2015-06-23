var Utils = function() {
	var logFlag = false;
	var CRC16_TABLE = [
		0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
		0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
		0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
		0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
		0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
		0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
		0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
		0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
		0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
		0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
		0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
		0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
		0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
		0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
		0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
		0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
		0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
		0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
		0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
		0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
		0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
		0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
		0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
		0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
		0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
		0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
		0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
		0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
		0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
		0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
		0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
		0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
	];

	var SHA256_TABLE = [
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
		0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
		0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
		0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
		0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
		0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
		0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
		0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
		0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
		0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
		0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
		0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
	];

	var SHA256_INIT = [
		0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
		0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
	];

	this.check_version = function(version) {
		return version.slice(0, 6) === '3U1504' ||
			version.slice(0, 6) === '4M1505';
	};

	this.crc16 = function(arraybuffer) {
		var data = new Uint8Array(arraybuffer);
		var crc = 0;
		var i = 0;
		var len = data.byteLength;

		while (len-- > 0)
			crc = CRC16_TABLE[((crc >>> 8) ^ data[i++]) & 0xff] ^ (crc << 8);

		return crc & 0xffff;
	};

	this.ab2hex = function(arraybuffer) {
		var view = new Uint8Array(arraybuffer);
		var str = '';
		for (var v of view)
			str += ('0' + v.toString(16)).slice(-2);
		return str;
	};

	this.hex2ab = function(hex) {
		var len = hex.length / 2;
		var arraybuffer = new ArrayBuffer(len);
		var view = new Uint8Array(arraybuffer);
		for (var i = 0; i < len; i++)
		view[i] = parseInt(hex.slice(i * 2, i * 2 + 2), 16);
		return arraybuffer;
	};

	this.ab2asc = function(arraybuffer) {
		return String.fromCharCode.apply(null, new Uint8Array(arraybuffer));
	};

	this.str2ab = function(str) {
		var len = str.length;
		var arraybuffer = new ArrayBuffer(len);
		var view = new Uint8Array(arraybuffer);
		for (var i = 0; i < len; i++)
		view[i] = str.charCodeAt(i);
		return arraybuffer;
	};

	this.gwPool2raw = function(midstat, data, poolId, jobId, nonce2) {
		var raw = new ArrayBuffer(64);
		var view = new DataView(raw);
		var i;

		data = data.slice(128, 128 + 24);
		for (i = 0; i < 32; i++)
		view.setUint8(31 - i, parseInt(midstat.slice(i * 2, i * 2 + 2), 16));
		view.setUint8(32, poolId);
		view.setUint8(33, jobId);
		view.setUint32(34, nonce2, false);
		for (i = 0; i < 12; i++)
		view.setUint8(63 - i, parseInt(data.slice(i * 2, i * 2 + 2), 16));
		return raw;
	};

	this.sha256 = function(hex) {
		var shaObj = new jsSHA(hex, "HEX");
		return shaObj.getHash("SHA-256", "HEX");
	};

	this.getBlockheader = function(job, nonce2) {
		nonce2 = this.uInt2LeHex(nonce2, job.nonce2Size);
		var coinbase = job.coinbase1 + job.nonce1 + nonce2 + job.coinbase2;
		var merkleRoot = this.sha256(this.sha256(coinbase));
		for (var branch of job.merkleBranch)
			merkleRoot = this.sha256(this.sha256(merkleRoot + branch));
		var arraybuffer = new ArrayBuffer(76);
		var view = new DataView(arraybuffer);
		view.setUint32(0, parseInt(job.version, 16));
		for (var i = 0; i < 8; i++) {
			view.setUint32(
				(i + 1) * 4,
				parseInt(job.prevhash.slice(i * 8, i * 8 + 8), 16)
			);
			view.setUint32(
				(i + 9) * 4,
				parseInt(merkleRoot.slice(i * 8, i * 8 + 8), 16),
				true
			);
		}
		view.setUint32(17 * 4, parseInt(job.ntime, 16));
		view.setUint32(18 * 4, parseInt(job.nbits, 16));

		return this.ab2hex(arraybuffer);
	};

	this.varifyWork = function(job, nonce2, ntime, nonce) {
		nonce2 = this.uInt2LeHex(nonce2, job.nonce2Size);
		var coinbase = job.coinbase1 + job.nonce1 + nonce2 + job.coinbase2;
		var merkleRoot = this.sha256(this.sha256(coinbase));
		for (var branch of job.merkleBranch)
			merkleRoot = this.sha256(this.sha256(merkleRoot + branch));
		var arraybuffer = new ArrayBuffer(80);
		var view = new DataView(arraybuffer);
		view.setUint32(0, parseInt(job.version, 16), true);
		for (var i = 0; i < 8; i++) {
			view.setUint32(
				(i + 1) * 4,
				parseInt(job.prevhash.slice(i * 8, i * 8 + 8), 16),
				true
			);
			view.setUint32(
				(i + 9) * 4,
				parseInt(merkleRoot.slice(i * 8, i * 8 + 8), 16)
			);
		}
		view.setUint32(17 * 4, ntime, true);
		view.setUint32(18 * 4, parseInt(job.nbits, 16), true);
		view.setUint32(19 * 4, nonce);

		var hash = this.sha256(this.sha256(this.ab2hex(arraybuffer)));
		if (hash.slice(56, 64) !== '00000000')
			// hard ware error
			return 2;
			var targetView = new Uint8Array(job.target);
			var hashView = new Uint8Array(this.hex2ab(hash));
			for (var j = 0; j < 32; j++) {
				if (hashView[31 - j] > targetView[j])
					// above target
					return 1;
				else if (hashView[31 - j] < targetView[j])
					return 0;
			}
			return 0;
	};

	this.getMidstate = function(data) {
		var view = new DataView(this.hex2ab(data.slice(0, 128)));
		var w = [], v = [];
		var i, s0, s1, ma, ch;
		for (i = 0; i < 16; i++)
			w[i] = view.getUint32(i * 4, true);
		for (i = 0; i < 8; i++)
			v[i] = SHA256_INIT[i];
		for (var k of SHA256_TABLE) {
			s0 = _rotateright(v[0], 2) ^ _rotateright(v[0], 13) ^ _rotateright(v[0], 22);
			s1 = _rotateright(v[4], 6) ^ _rotateright(v[4], 11) ^ _rotateright(v[4], 25);
			ma = (v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]);
			ch = (v[4] & v[5]) ^ ((~v[4]) & v[6]);

			v[7] = _addu32(v[7], w[0], k, ch, s1);
			v[3] = _addu32(v[3], v[7]);
			v[7] = _addu32(v[7], ma, s0);

			v.unshift(v[7]);
			v.pop();

			s0 = _rotateright(w[1], 7) ^ _rotateright(w[1], 18) ^ (w[1] >>> 3);
			s1 = _rotateright(w[14], 17) ^ _rotateright(w[14], 19) ^ (w[14] >>> 10);

			w.push(_addu32(w[0], s0, w[9], s1));
			w.shift();
		}

		var arraybuffer = new ArrayBuffer(32);
		var new_view = new DataView(arraybuffer);
		for (i = 0; i < 8; i++)
		new_view.setUint32(i * 4, _addu32(v[i], SHA256_INIT[i]), true);
		return this.ab2hex(arraybuffer);
	};

	var _rotateright = function(i, p) {
		p &= 0x1f;
		return i >>> p | ((i << (32 - p)) & 0xffffffff);
	};

	var _addu32 = function() {
		var sum = 0;
		for (var i of arguments) {
			sum = (sum + i) & 0xffffffff;
		}
		return sum & 0xffffffff;
	};

	this.padLeft = function(str, len) {
		len = len || 4;
		len *= 2;
		while (str.length < len)
			str = "0" + str;
		return str;
	};

	this.uInt2LeHex = function(integar, size) {
		var arraybuffer = new ArrayBuffer(4);
		var view = new DataView(arraybuffer);
		view.setUint32(0, integar, true);
		return this.ab2hex(arraybuffer).slice(0, size * 2);
	};

	this.arraySum = function(array) {
		return array.reduce(function(a, b) {return a + b;});
	};

	this.getTarget = function(diff) {
		// only work with diff lower than 0xffff
		var arraybuffer = new ArrayBuffer(32);
		var view = new DataView(arraybuffer);
		var base = 0xffff;
		for (var i = 2; i < 16; i++) {
			view.setUint16(i * 2, Math.floor(base / diff));
			base = (base % diff) << 16;
		}
		return arraybuffer;
	};

	this.log = function(level, args, header, style) {
		if (!logFlag)
			return;
		style = style || "";
		var template = "%c[" + header + "] " + args.shift();
		args.unshift(style);
		args.unshift(template);
		switch (level) {
		case "error":
			console.error.apply(console, args);
			break;
		case "warn":
			console.warn.apply(console, args);
			break;
		case "info":
			console.info.apply(console, args);
			break;
		case "log":
			console.log.apply(console, args);
			break;
		case "debug":
			console.debug.apply(console, args);
			break;
		}
	};

	this.enableLog = function() {
		logFlag = true;
	};
	this.disableLog = function() {
		logFlag = false;
	};
};

var utils = new Utils();
