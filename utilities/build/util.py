#
# Copyright (C) 2011 Alex Smith
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

## Helper for creating source lists with certain files only enabled by config settings.
# @param config		Configuration object.
# @param files		List of files. If an entry is a string, it is always
#			added. If a tuple, the first entry is the config
#                       setting and the second is the file name, and will only
#			be added if the setting is True.
def FeatureSources(config, files):
	from SCons.Script import File
	
	output = []
	for f in files:
		if type(f) == tuple:
			if config[f[0]]:
				output.append(File(f[1]))
		else:
			output.append(File(f))
	return output
