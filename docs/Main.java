import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.regex.Pattern;

public class Main {

    private static Pattern FUNC_HEADER_PATTERN = Pattern.compile(" ([a-zA-Z0-9]+)\\(");

    private static boolean isFuncHeader(String line) {
        // this fails in the case of default args
        //if (line.contains("="))
            //return false;
        int eqIdx = line.indexOf(" = ");
        int parenIdx = line.indexOf("(");
        if (eqIdx != -1 && eqIdx < parenIdx)
            return false;

        return FUNC_HEADER_PATTERN.matcher(line).find();
    }

    public static List<String> Process(List<String> lines, List<String> publicForwardDeclarations) {

        List<String> privateForwardDeclarations = new ArrayList<>(); // private

        List<String> result = new ArrayList<>();

        int whereToPlacePrivateDeclarations = -1;

        // Removal of entries
        lines.removeIf(s -> (s.trim().contains("using") && s.trim().endsWith(";"))
                || s.trim().startsWith("//"));

        // Condensing/merging entries
        {
            List<String> copy = new ArrayList<>();
            for (int i=0; i < lines.size(); i++) {
                String line = lines.get(i).replace("\t", "    ");
                final String trimmed = line.trim();

                if (trimmed.startsWith("{")) { // condense opening brackets
                    copy.set(copy.size() - 1, copy.get(copy.size() - 1) + " {");
                } else
                    copy.add(line);
            }
            lines = copy;
        }

        // Formatting of entries
        for (int i=0; i < lines.size(); i++) {
            String line = lines.get(i).replace("\t", "    ");

            {
                final String trimmed = line.trim();

                if (whereToPlacePrivateDeclarations == -1) {
                    if (trimmed.endsWith("{"))
                        whereToPlacePrivateDeclarations = result.size() + 1;
                }



                // template params too complicated
                if (!(line.contains("<") || line.contains("ref "))) {
                    // replace c# out keyword with c++ & reference
                    if (line.contains("out ")) {
                        // remove the out,
                        // add & between type and name

                        int indexOfOut = line.indexOf("out ");
                        line = line.replace("out ", "");

                        // test if this is a function declaration
                        if (isFuncHeader(line)) {
                            // add &
                            String sub = line.substring(indexOfOut, line.indexOf(" ", indexOfOut) + 1);
                            String ref = sub + "&";
                            line = line.replaceFirst(sub, ref);
                        }
                    }

                    if (isFuncHeader(line)) {
                        // replace string, Vector3, Quaternion, Vector2i, with const&
                        int delimiterStart = line.indexOf("(") + 1;
                        String[] args = line.substring(delimiterStart, line.lastIndexOf(")")).split(", ");
                        // now format
                        for (final String originalArg : args) {
                            if (originalArg.isEmpty())
                                break;

                            if (originalArg.contains("out "))
                                continue;

                            String arg = originalArg;
                            if (arg.contains("Vector3") ||
                                    arg.contains("string") ||
                                    arg.contains("Quaternion") ||
                                    arg.contains("Vector2i")) {
                                // add const & between words
                                int spaceIdx = arg.indexOf(" ");
                                arg = arg.replace(arg.substring(spaceIdx, spaceIdx + 1), " &");
                                arg = "const " + arg;

                                line = line.replace(originalArg, arg);
                            }
                        }
                        // now reassign line
                    }

                }

                // replace only if there is non space, non letter, or non letter nearby on both sides
                line = line.replace("(int)", "(int32_t)");
                line = line.replace("(long)", "(int64_t)");
                line = line.replace("ZPackage", "NetPackage");

                line = line.replace(" int ", " int32_t ");
                line = line.replace(" long ", " int64_t ");

                line = line.replace("(int ", "(int32_t ");
                line = line.replace("(long ", "(int64_t ");

                line = line.replace("int64_t sender", "OWNER_t sender");
                line = line.replace("int64_t peer", "OWNER_t peer");

                line = line.replace("string", "std::string");

                if (line.contains("ZLog.Log(")) { // replace logger
                    // replace
                    line = line.replace("ZLog.Log(", "LOG(INFO) << ")
                            .replace("+", "<<")
                            .replace(");", ";");
                }

                if (line.contains("static ")) {
                    line = line.replace("static ", "");
                }

                if (line.contains("List<")) { // List<string> list = new List<string>(this.m_globalKeys);
                    if (trimmed.contains(" = new List<") && trimmed.endsWith("();"))
                        line = line.substring(0, line.indexOf(" = ")) + ";"; // keep the first part

                    line = line.replace("List<", "std::vector<");
                }

                if (line.contains("Dictionary<")) { // List<string> list = new List<string>(this.m_globalKeys);
                    if (line.contains(" = new Dictionary<") && trimmed.endsWith("();"))
                        line = line.substring(0, line.indexOf(" = ")) + ";";

                    line = line.replace("Dictionary<", "robin_hood::unordered_map<");
                }

                if (trimmed.startsWith("foreach")) {
                    line = line.replace("foreach", "for");
                    int indexOfInKeyword = line.indexOf(" in ");
                    int spaceBeforeVarName = line.lastIndexOf(" ", indexOfInKeyword - 1);

                    // replace ' in '
                    line = line.replace(" in ", " : ");

                    // replace with auto&&
                    line = line.replace(line.substring(line.indexOf("(") + 1, spaceBeforeVarName), "auto&&");
                }

                if (trimmed.startsWith("public") && trimmed.endsWith(") {")) { // public fn
                    line = line.replace("public ", "");
                    publicForwardDeclarations.add(line.trim().replace(" {", "") + ";");
                    result.add("    // public");
                }

                if (trimmed.startsWith("private") && trimmed.endsWith(") {")) { // private fn
                    line = line.replace("private ", "");
                    privateForwardDeclarations.add("    " + line.trim().replace(" {", "") + ";");
                    result.add("    // private");
                }
            }

            result.add(line);
        }

        privateForwardDeclarations.add("");
        result.addAll(whereToPlacePrivateDeclarations, privateForwardDeclarations);

        return result;
    }

    public static void main(String[] args) throws IOException {

        // remove all comment lines

        // condense all newline starting brackets '{' to previous line

        // remove all public/private specifiers, but sort them ?

        // create forward declarations for private classes

        // create public forward declarations for header

        // convert ZLog to LOG(INFO)...

        // convert list declarations to vector

        // convert 'foreach ... in' to for

        List<String> lines = Files.readAllLines(Paths.get("C:/Users/Rico/Documents/Valheim0_212_6_Project/assembly_valheim/Heightmap.cs"));

        List<String> publicForwardDeclarations = new ArrayList<>();
        List<String> convertedSourceFile = Process(lines, publicForwardDeclarations);

        // emit file
        FileWriter writer = new FileWriter("emitted.cpp");
        for(String str : convertedSourceFile) {
            writer.write(str + System.lineSeparator());
        }
        writer.close();

        for (String f : publicForwardDeclarations)
            System.out.println(f);

    }

}
