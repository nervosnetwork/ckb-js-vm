import { Command } from "commander";
import fs from "fs-extra";
import path from "path";
import updateCheck from "update-check";
import prompts from "prompts";
import { bold, cyan, green, red, yellow } from "picocolors";
import validateProjectName from "validate-npm-package-name";
import { spawn } from "child_process";
import {
  bindingVersion,
  cccCoreVersion,
  coreVersion,
  eslintPluginVersion,
  testtoolVersion,
} from "./config";

const packageJson = require(path.join(__dirname, "../package.json"));

let projectName = "";

const handleSigTerm = (): void => process.exit(0);

process.on("SIGINT", handleSigTerm);
process.on("SIGTERM", handleSigTerm);

const onPromptState = (state: any): void => {
  if (state.aborted) {
    // If we don't re-enable the terminal cursor before exiting
    // the program, the cursor will remain hidden
    process.stdout.write("\x1B[?25h");
    process.stdout.write("\n");
    process.exit(1);
  }
};

const program = new Command(packageJson.name)
  .version(
    packageJson.version,
    "-v, --version",
    `Output the current version of ${packageJson.name}.`,
  )
  .argument("[directory]")
  .usage("[directory] [options]")
  .helpOption("-h, --help", "Display this help message.")
  .option(
    "--skip-install",
    "Explicitly tell the CLI to skip installing packages.",
  )
  .option(
    "--use-npm",
    "Explicitly tell the CLI to generate npm packages(default pnpm)",
  )
  .option(
    "--add <package name>",
    "Add on-chain script package to current project",
  )
  .action((name: string) => {
    // Commander does not implicitly support negated options. When they are used
    // by the user they will be interpreted as the positional argument (name) in
    // the action handler. See https://github.com/tj/commander.js/pull/1355
    if (name && !name.startsWith("--no-")) {
      projectName = name;
    }
  })
  .allowUnknownOption()
  .parse(process.argv);

const opts = program.opts();

function getPkgManager(): string {
  if (opts.useNpm) {
    return "npm";
  } else {
    return "pnpm";
  }
}

const packageManager = getPkgManager();

interface ValidationResult {
  valid: boolean;
  problems?: string[];
}

function validateNpmName(name: string): ValidationResult {
  const nameValidation = validateProjectName(name);
  if (nameValidation.validForNewPackages) {
    return { valid: true };
  }

  return {
    valid: false,
    problems: [
      ...(nameValidation.errors || []),
      ...(nameValidation.warnings || []),
    ],
  };
}

function isFolderEmpty(folderPath: string): boolean {
  const files = fs.readdirSync(folderPath);
  return files.length === 0;
}

export async function install(packageManager: string): Promise<void> {
  let args = ["install"];
  if (packageManager === "pnpm") {
    // resolve following issue:
    //  ERR_PNPM_OUTDATED_LOCKFILE  Cannot install with "frozen-lockfile" because pnpm-lock.yaml is not up to date with <ROOT>/packages/another-script/package.json
    args = ["install", "--no-frozen-lockfile"];
  }
  return new Promise((resolve, reject) => {
    const child = spawn(packageManager, args, {
      stdio: "inherit",
      env: {
        ...process.env,
        ADBLOCK: "1",
        // we set NODE_ENV to development as pnpm skips dev
        // dependencies when production
        NODE_ENV: "development",
        DISABLE_OPENCOLLECTIVE: "1",
      },
    });
    child.on("close", (code) => {
      if (code !== 0) {
        reject({ command: `${packageManager} ${args.join(" ")}` });
        return;
      }
      resolve();
    });
  });
}

function updateProjectPackage(projectPath: string, packageName?: string) {
  // Use packageName if provided, otherwise use the global projectName
  const targetPackageName = packageName || projectName;
  const packageJsonPath = path.join(
    projectPath,
    packageName
      ? `packages/${packageName}/package.json`
      : "packages/on-chain-script/package.json",
  );
  let json: any = "";
  if (fs.pathExistsSync(packageJsonPath)) {
    try {
      json = fs.readJsonSync(packageJsonPath);
      json.name = targetPackageName;
      json.devDependencies["@ckb-js-std/eslint-plugin"] = eslintPluginVersion;
      json.dependencies["@ckb-js-std/bindings"] = bindingVersion;
      json.dependencies["@ckb-js-std/core"] = coreVersion;
      fs.writeJsonSync(packageJsonPath, json, { spaces: 2 });
      console.log(green(`Updated ${targetPackageName}/package.json.`));
    } catch (error: any) {
      console.error(
        red(
          `Failed to update ${packageName ? `packages/${packageName}` : "packages/on-chain-script"}/package.json: ${error.message}`,
        ),
      );
      process.exit(1);
    }
  } else {
    console.error(
      red(
        `Could not find ${packageName ? `packages/${packageName}` : "packages/on-chain-script"}/package.json in the template. Make sure your template includes a package.json.`,
      ),
    );
    process.exit(1);
  }
}

function updateTestPackage(projectPath: string) {
  const packageJsonPath = path.join(
    projectPath,
    "packages/on-chain-script-tests/package.json",
  );
  let json: any = "";
  if (fs.pathExistsSync(packageJsonPath)) {
    try {
      json = fs.readJsonSync(packageJsonPath);
      json.name = projectName + "-tests";
      json.devDependencies["@ckb-ccc/core"] = cccCoreVersion;

      fs.writeJsonSync(packageJsonPath, json, { spaces: 2 });
      console.log(green(`Updated ${projectName}/package.json.`));
    } catch (error: any) {
      console.error(
        red(
          `Failed to update packages/on-chain-script/package.json: ${error.message}`,
        ),
      );
      process.exit(1);
    }
  } else {
    console.error(
      red(
        `Could not find packages/on-chain-script/package.json in the template. Make sure your template includes a package.json.`,
      ),
    );
    process.exit(1);
  }
}

function updateRootPackage(projectPath: string) {
  const packageJsonPath = path.join(projectPath, "package.json");
  let json: any = "";
  if (fs.pathExistsSync(packageJsonPath)) {
    try {
      json = fs.readJsonSync(packageJsonPath);
      json.devDependencies["ckb-testtool"] = testtoolVersion;

      // Update scripts to use npm workspaces commands if using npm
      if (getPkgManager() === "npm") {
        json.scripts = {
          build: "npm run build --workspaces --if-present",
          clean: "npm run clean --workspaces --if-present",
          test: "npm run test --workspaces --if-present",
          format: "npm run format --workspaces --if-present",
          lint: "npm run lint --workspaces --if-present",
        };

        // Remove pnpm specific configuration
        if (json.pnpm) {
          delete json.pnpm;
        }
      }

      fs.writeJsonSync(packageJsonPath, json, { spaces: 2 });
      console.log(green(`Updated root package.json.`));
    } catch (error: any) {
      console.error(
        red(`Failed to update root package.json: ${error.message}`),
      );
      process.exit(1);
    }
  } else {
    console.error(
      red(
        `Could not find package.json in the template. Make sure your template includes a package.json.`,
      ),
    );
    process.exit(1);
  }
}

async function run(): Promise<void> {
  console.log();
  if (projectName && typeof projectName === "string") {
    projectName = projectName.trim();
  }

  if (!projectName) {
    const res = await prompts({
      onState: onPromptState,
      type: "text",
      name: "path",
      message: "What is your project named?",
      initial: "my-ckb-script",
      validate: (name) => {
        const validation = validateNpmName(path.basename(path.resolve(name)));
        if (validation.valid) {
          return true;
        }
        return (
          "Invalid project name: " +
          (validation.problems?.[0] || "Unknown validation error")
        );
      },
    });

    if (typeof res.path === "string") {
      projectName = res.path.trim();
    }
  }

  if (!projectName) {
    console.log(
      "\nPlease specify the project directory:\n" +
        `  ${cyan(program.name())} ${green("<project-directory>")}\n` +
        "For example:\n" +
        `  ${cyan(program.name())} ${green("my-ckb-script")}\n\n` +
        `Run ${cyan(`${program.name()} --help`)} to see all options.`,
    );
    process.exit(1);
  }

  const appPath = path.resolve(projectName);
  const appName = path.basename(appPath);

  const validation = validateNpmName(appName);
  if (!validation.valid) {
    console.error(
      `Could not create a project called ${red(
        `"${appName}"`,
      )} because of npm naming restrictions:`,
    );

    validation.problems?.forEach((p) =>
      console.error(`    ${red(bold("*"))} ${p}`),
    );
    process.exit(1);
  }

  if (fs.pathExistsSync(appPath) && !isFolderEmpty(appPath)) {
    console.error(
      `Could not create a project called ${red(
        `"${appName}"`,
      )} because a project with the same name already exists.`,
    );
    process.exit(1);
  }

  console.log(bold(`Using ${packageManager}.`));

  const templatePath = path.join(__dirname, `../templates`);
  if (!fs.pathExistsSync(templatePath)) {
    console.error(`Could not find a template`);
    console.error(`\n 😮‍💨 Project ${projectName} created failed!\n`);
    process.exit(1);
  }

  const originalDirectory = process.cwd();
  const projectPath = path.join(originalDirectory, projectName);

  fs.ensureDirSync(projectPath);
  fs.copySync(templatePath, projectPath);

  updateRootPackage(projectPath);
  updateProjectPackage(projectName);
  updateTestPackage(projectName);

  console.log(`\n🎉 Project ${projectName} created!\n`);

  if (opts.skipInstall) {
    console.log(
      "Skip install the dependencies, we suggest that you begin by typing:",
    );
    console.log();
    console.log(cyan("  cd"), projectName);
    console.log(`  ${cyan(`${packageManager} install`)}`);
    console.log();
  } else {
    console.log("\nInstalling dependencies:");
    console.log();
    console.log("Installing packages. This might take a couple of minutes.");
    console.log();

    process.chdir(appPath);
    await install(packageManager);
    console.log("Packages installed.");
    console.log();
  }

  console.log(`${green("Success!")} Created ${projectName} at ${projectPath}`);
  console.log();
}

const update = updateCheck(packageJson).catch(() => null);

async function notifyUpdate(): Promise<void> {
  try {
    const updateInfo = await update;
    if (updateInfo?.latest) {
      const global: Record<string, string> = {
        pnpm: "pnpm add",
        npm: "npm update",
      };
      const updateMessage = `${global[packageManager]} ${packageJson.name}`;
      console.log(
        yellow(bold(`A new version of \`${packageJson.name}\` is available!`)) +
          "\n" +
          "You can update by running: " +
          cyan(updateMessage) +
          "\n",
      );
    }
  } catch {
    // ignore error
  }
}

interface ExitReason {
  command?: string;
  message?: string;
}

async function exit(reason: ExitReason): Promise<void> {
  console.log();
  console.log("Aborting installation.");
  if (reason.command) {
    console.log(`  ${cyan(reason.command)} has failed.`);
  } else {
    console.log(
      red("Unexpected error. Please report it as a bug:") + "\n",
      reason,
    );
  }
  console.log();
  process.exit(1);
}

async function add(): Promise<void> {
  // Step 1: Check that package.json exists in current directory
  const currentDir = process.cwd();
  const packageJsonPath = path.join(currentDir, "package.json");

  if (!fs.pathExistsSync(packageJsonPath)) {
    console.error(red("Failed: package.json not found in current directory."));
    process.exit(1);
  }

  // Get the package name from the --add option
  const packageName = opts.add;
  if (!packageName || typeof packageName !== "string") {
    console.error(
      red("Failed: package name is required when using --add option."),
    );
    process.exit(1);
  }

  // Validate the package name
  const validation = validateNpmName(packageName);
  if (!validation.valid) {
    console.error(
      `Could not create a package called ${red(
        `"${packageName}"`,
      )} because of npm naming restrictions:`,
    );
    validation.problems?.forEach((p) =>
      console.error(`    ${red(bold("*"))} ${p}`),
    );
    process.exit(1);
  }

  // Step 2: Copy templates/packages/on-chain-script to packages/new-name
  const templateSourcePath = path.join(
    __dirname,
    "../templates/packages/on-chain-script",
  );
  const targetPath = path.join(currentDir, "packages", packageName);

  if (!fs.pathExistsSync(templateSourcePath)) {
    console.error(red(`Failed: Template not found at ${templateSourcePath}`));
    process.exit(1);
  }

  // Check if target directory already exists
  if (fs.pathExistsSync(targetPath)) {
    console.error(
      red(`Failed: Package directory ${targetPath} already exists.`),
    );
    process.exit(1);
  }

  // Ensure the packages directory exists
  const packagesDir = path.join(currentDir, "packages");
  fs.ensureDirSync(packagesDir);

  // Copy the template
  try {
    fs.copySync(templateSourcePath, targetPath);
    console.log(green(`Copied template to packages/${packageName}`));
  } catch (error: any) {
    console.error(red(`Failed to copy template: ${error.message}`));
    process.exit(1);
  }

  // Step 3: Call updateProjectPackage on it
  try {
    updateProjectPackage(currentDir, packageName);
    console.log(green(`✅ Successfully added package "${packageName}"`));
  } catch (error: any) {
    console.error(red(`Failed to update package: ${error.message}`));
    process.exit(1);
  }

  // Step 4: install dependencies
  console.log("\nInstalling dependencies:");
  await install(packageManager);
  console.log("Packages installed.");
  console.log();
}

(async () => {
  try {
    // Check if --add option is provided
    if (opts.add) {
      await add();
    } else {
      await run();
    }
    await notifyUpdate();
  } catch (error) {
    await exit(error as ExitReason);
  }
})();
