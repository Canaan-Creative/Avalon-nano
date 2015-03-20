chrome.app.runtime.onLaunched.addListener(function() {
	chrome.app.window.create('index.html', {
		innerBounds: {
		//bounds: {
			width: 670,
			height: 350,
			minWidth: 670,
			minHeight: 350
		},
	id: "Avalon-Nano"
	});
});
