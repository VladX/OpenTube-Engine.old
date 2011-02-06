static const uchar http_mime_types_len[] = {9, 9, 9, 8, 24, 9, 9, 10, 10, 12, 10, 13, 8, 29, 11, 10, 24};

const uchar * http_mime_types[] = {
	
	/* File extension | Mime type | Mime type length */
	
	(const uchar *) ".html", (const uchar *) "text/html", http_mime_types_len,
	(const uchar *) ".htm", (const uchar *) "text/html", http_mime_types_len + 1,
	(const uchar *) ".shtml", (const uchar *) "text/html", http_mime_types_len + 2,
	(const uchar *) ".css", (const uchar *) "text/css", http_mime_types_len + 3,
	(const uchar *) ".js", (const uchar *) "application/x-javascript", http_mime_types_len + 4,
	(const uchar *) ".png", (const uchar *) "image/png", http_mime_types_len + 5,
	(const uchar *) ".gif", (const uchar *) "image/gif", http_mime_types_len + 6,
	(const uchar *) ".jpg", (const uchar *) "image/jpeg", http_mime_types_len + 7,
	(const uchar *) ".jpeg", (const uchar *) "image/jpeg", http_mime_types_len + 8,
	(const uchar *) ".ico", (const uchar *) "image/x-icon", http_mime_types_len + 9,
	(const uchar *) ".txt", (const uchar *) "text/plain", http_mime_types_len + 10,
	(const uchar *) ".svg", (const uchar *) "image/svg+xml", http_mime_types_len + 11,
	(const uchar *) ".xml", (const uchar *) "text/xml", http_mime_types_len + 12,
	(const uchar *) ".swf", (const uchar *) "application/x-shockwave-flash", http_mime_types_len + 13,
	(const uchar *) ".flv", (const uchar *) "video/x-flv", http_mime_types_len + 14,
	(const uchar *) ".mp3", (const uchar *) "audio/mpeg", http_mime_types_len + 15,
	
	(const uchar *) "", (const uchar *) "application/octet-stream", http_mime_types_len + 16
};
