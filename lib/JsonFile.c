// Reads a JSON file, on 1.29 and 1.30 alike. Deserialise at the call site with a typed
// variable:
//     string text, error;
//     if (JsonFile.Read(path, text, error)) { new JsonSerializer().ReadFromString(typedObj, text, error); }
// (JsonFileLoader<T> won't take mission-script classes on 1.29, and JsonSerializer
// fills nothing when the object is passed through a `Class` parameter.)
class JsonFile
{
	static bool Read(string path, out string content, out string error)
	{
		// "./mpmissions/x/file" works as is on 1.30; 1.29 wants the $CurrentDir: prefix
		if (!FileExist(path))
		{
			string alt = path;
			if (alt.IndexOf("./") == 0)
				alt = alt.Substring(2, alt.Length() - 2);
			alt = "$CurrentDir:" + alt;
			if (FileExist(alt))
				path = alt;
		}
		if (!FileExist(path))
		{
			error = "file " + path + " does not exist";
			return false;
		}
		FileHandle handle = OpenFile(path, FileMode.READ);
		if (handle == 0)
		{
			error = "cannot open " + path;
			return false;
		}
		ReadFile(handle, content, 100000000);
		CloseFile(handle);
		return true;
	}
}
