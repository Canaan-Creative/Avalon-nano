chrome.app.runtime.onLaunched.addListener(function() {
	chrome.app.window.create('index.html', {
		innerBounds: {
		//bounds: {
			width: 870,
			height: 700,
			minWidth: 870,
			minHeight: 700
		},
	    id: "Avalon Nano"
	});
});
