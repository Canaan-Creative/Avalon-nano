chrome.app.runtime.onLaunched.addListener(function() {
	chrome.app.window.create('index.html', {
		innerBounds: {
		//bounds: {
			width: 570,
			height: 250,
			minWidth: 570,
			minHeight: 250
		},
	    id: "Avalon Nano"
	});
});
